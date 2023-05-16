#include "UserEventsManager.h"

#include "NativeInterface.h"
#include "util.h"
using namespace std;

static int emptyCallback(void* input, int argc, char** argv, char** azColName) { return 0; }

static int get_schema_call_back(void* data, int argc, char** argv, char** azColName) {
  if (argc == 0) {
    return 0;
  }
  std::string* output = (std::string*)data;
  if (!strcmp(azColName[0], "sql")) {
    *output = argv[0];
  } else {
    *output = "";
  }
  return 0;
}

static int add_rows_to_table_store(void* data, int argc, char** argv, char** azColName) {
  TableStore* tableStore = (TableStore*)data;
  TableRow r;
  bool foundTimestamp = false;
  for (int i = 0; i < argc; i++) {
    if (!strcmp(azColName[i], "TIMESTAMP")) {
      r.timestamp = stoll(argv[i]);
      foundTimestamp = true;
      continue;
    }
    r.row[azColName[i]] = argv[i] ? argv[i] : "NULL";
  }
  if (foundTimestamp)
    tableStore->add_row(r);
  else
    LOG_TO_ERROR("%s", "No timestamp found in table row");
  return 0;
}

UserEventsManager::UserEventsManager(const std::vector<json>& tableInfos, bool debug)
    : _debugMode(debug) {
  char* zErrMsg = 0;
  int rc;
  rc = sqlite3_open((NativeInterface::HOMEDIR + DEFAULT_SQLITE_DB_NAME).c_str(), &_db);
  if (rc) {
    LOG_TO_ERROR("Can't open database: %s %s", DEFAULT_SQLITE_DB_NAME, sqlite3_errmsg(_db));
  } else {
    LOG_TO_INFO("Opened database=%s successfully", DEFAULT_SQLITE_DB_NAME);
  }
  for (int i = 0; i < tableInfos.size(); i++) {
    if (!create_table(tableInfos[i])) {
      LOG_TO_ERROR("Could not create TableId=%d", i);
    }
  }
}

template <class T>
bool UserEventsManager::create_preprocessor(const std::string& modelInputIdentifier,
                                            const json& preprocessorJson) {
  PreProcessorInfo<T> info = JSONParser::get_from_json<PreProcessorInfo<T>>(preprocessorJson);
  if (!info.valid) {
    LOG_TO_ERROR("PreprocessorInfo could not be parsed for modelInput=%s",
                 modelInputIdentifier.c_str());
    return false;
  }
  if (_tableStoreMap.find(info.tableName) == _tableStoreMap.end()) {
    LOG_TO_ERROR("TableStore does not exist for %s", info.tableName.c_str());
    return false;
  }
  if (!_tableStoreMap[info.tableName].is_table_ready_to_be_filled()) {
    _tableStoreMap[info.tableName].set_to_fill_in_memory();
    char* zErrMsg = 0;
    string query = "SELECT * from " + info.tableName + " ORDER BY TIMESTAMP";
    int rc = sqlite3_exec(_db, query.c_str(), add_rows_to_table_store,
                          &_tableStoreMap[info.tableName], &zErrMsg);
    if (rc != SQLITE_OK) {
      LOG_TO_ERROR("SQL error: %s in running query=%s", zErrMsg, query.c_str());
      sqlite3_free(zErrMsg);
      return false;
    }
  }
  _modelInputToTableNameMap[modelInputIdentifier] = info.tableName;
  bool ret = _tableStoreMap[info.tableName].create_preprocessor(modelInputIdentifier, info);
  return ret;
}

string UserEventsManager::get_existing_create_table_command(const std::string& tableName) {
  string existingTableCommand = "";
  char* zErrMsg = 0;
  char* getExistingTableCommand;
  asprintf(&getExistingTableCommand, "SELECT sql FROM sqlite_schema where name='%s';",
           tableName.c_str());
  int rc = sqlite3_exec(_db, getExistingTableCommand, get_schema_call_back, &existingTableCommand,
                        &zErrMsg);
  free(getExistingTableCommand);
  if (rc != SQLITE_OK) {
    LOG_TO_ERROR(
        "Error in get_existing_create_table_command Table %s with different schema with error %s",
        tableName.c_str(), zErrMsg);
    sqlite3_free(zErrMsg);
    return "";
  }
  return existingTableCommand;
}

bool UserEventsManager::create_table_in_db(const TableInfo& tableInfo) {
  char* zErrMsg = 0;
  const char* tableCreateCommand = "CREATE TABLE %s (TIMESTAMP INT%s) STRICT;";
  string tableColumnFields;
  string tableName = tableInfo.name;
  for (auto& it : tableInfo.schema) {
    tableColumnFields += ", " + it.first + " " + it.second;
  }
  char* sqlQueryCommand;
  asprintf(&sqlQueryCommand, tableCreateCommand, tableInfo.name.c_str(), tableColumnFields.c_str());
  std::string existingCreateTableCommand = get_existing_create_table_command(tableInfo.name);
  if (existingCreateTableCommand != "") {
    // table with same name already exists
    if (!strcmp((existingCreateTableCommand + ";").c_str(), sqlQueryCommand)) {
      // table schema is same as before
      free(sqlQueryCommand);
      return true;
    }
    // table schema has changed
    if (!_debugMode) {
      LOG_TO_ERROR("Schema of Table=%s has changed, can only change schema in debug mode",
                   tableInfo.name.c_str());
      free(sqlQueryCommand);
      return false;
    }
    // dropping existing table with different schema in debug mode
    char* dropTableCommand;
    asprintf(&dropTableCommand, "Drop table %s;", tableInfo.name.c_str());
    int rc = sqlite3_exec(_db, dropTableCommand, emptyCallback, 0, &zErrMsg);
    free(dropTableCommand);
    if (rc != SQLITE_OK) {
      LOG_TO_ERROR("Error in Dropping Table %s with different schema with error %s",
                   tableInfo.name.c_str(), zErrMsg);
      sqlite3_free(zErrMsg);
      free(sqlQueryCommand);
      return false;
    }
    LOG_TO_INFO("Dropped existing table=%s, with older schema", tableInfo.name.c_str());
  }
  int rc = sqlite3_exec(_db, sqlQueryCommand, emptyCallback, 0, &zErrMsg);
  free(sqlQueryCommand);
  if (rc != SQLITE_OK) {
    LOG_TO_ERROR("Error in Creating Table %s  with schema %s with error %s", tableInfo.name.c_str(),
                 tableColumnFields.c_str(), zErrMsg);
    sqlite3_free(zErrMsg);
    return false;
  }
  return true;
}

bool UserEventsManager::delete_old_rows_from_table_in_db(const TableInfo& tableInfo) {
  char* zErrMsg = 0;
  const char* tableDeleteRowsCommand = "DELETE FROM %s where TIMESTAMP <%ld";
  char* sqlQueryCommand;
  long expiryTimestamp = time(NULL) - tableInfo.expiryTimeInMins * 60;
  asprintf(&sqlQueryCommand, tableDeleteRowsCommand, tableInfo.name.c_str(), expiryTimestamp);
  int rc = sqlite3_exec(_db, sqlQueryCommand, emptyCallback, 0, &zErrMsg);
  free(sqlQueryCommand);
  if (rc != SQLITE_OK) {
    LOG_TO_ERROR("Error in Deleting old rows from Table %s  with expiryTimeStamp=%ld",
                 tableInfo.name.c_str(), expiryTimestamp, zErrMsg);
    sqlite3_free(zErrMsg);
    return false;
  }
  LOG_TO_DEBUG("Deleted old rows from Table=%s in DB successfully", tableInfo.name.c_str());

  return true;
}

bool UserEventsManager::create_table(const json& tableInfoJson) {
  TableInfo tableInfo = JSONParser::get_from_json<TableInfo>(tableInfoJson);
  if (!tableInfo.valid) {
    LOG_TO_ERROR("TableInfo=%s could not be parsed", tableInfoJson.dump().c_str());
    return false;
  }
  if (!create_table_in_db(tableInfo)) {
    return false;
  }
  // TODO: fix constructor call, remove TableStore default constructor
  _tableStoreMap[tableInfo.name] = TableStore(tableInfo);
  LOG_TO_INFO("Created Table=%s in in-memory DB successfully", tableInfo.name.c_str());

  if (!delete_old_rows_from_table_in_db(tableInfo)) {
    LOG_TO_ERROR("Could not delete old rows from the table %s ", tableInfo.name.c_str());
  }
  return true;
}

std::string get_value_as_string(const json& j) {
  stringstream ss;
  if (j.is_string())
    ss << j.get<std::string>();
  else
    ss << j;
  return ss.str();
}

bool UserEventsManager::add_event(const string& eventMapJsonString, const string& tableName) {
  if (_tableStoreMap.find(tableName) == _tableStoreMap.end()) {
    LOG_TO_ERROR("add_event failed, table=%s does not exist", tableName.c_str());
    return false;
  }
  char* zErrMsg = 0;
  nlohmann::json eventMapTable;
  try {
    eventMapTable = nlohmann::json::parse(eventMapJsonString.c_str());
  } catch (json::exception& e) {
    LOG_TO_ERROR("Error in parsing event for table:%s with eventMap: %s with error: %s",
                 tableName.c_str(), eventMapJsonString.c_str(), e.what());
    return false;
  } catch (...) {
    LOG_TO_ERROR("Error in parsing event for table:%s with eventMap: %s.", tableName.c_str(),
                 eventMapJsonString.c_str());
    return false;
  }
  const char* tableInserCommand = "INSERT INTO %s (TIMESTAMP %s) VALUES ('%ld' %s);";
  string tableColumnFields;
  string tableValueFields;
  TableRow r;
  r.timestamp = time(NULL);
  for (const auto& column : eventMapTable.items()) {
    tableColumnFields += " ," + column.key() + " ";
    string value = get_value_as_string(eventMapTable[column.key()]);
    tableValueFields += ", '" + value + "' ";
    r.row[column.key()] = value;
  }
  if (!_tableStoreMap[tableName].check_columns_of_row(r)) {
    return false;
  }
  char* sqlQueryCommand;
  asprintf(&sqlQueryCommand, tableInserCommand, tableName.c_str(), tableColumnFields.c_str(),
           long(time(NULL)), tableValueFields.c_str());
  int rc = sqlite3_exec(_db, sqlQueryCommand, emptyCallback, 0, &zErrMsg);
  free(sqlQueryCommand);
  if (rc != SQLITE_OK) {
    LOG_TO_ERROR("Error in Inserting element to  Table %s  with values %s with error %s",
                 tableName.c_str(), eventMapJsonString.c_str(), zErrMsg);
    sqlite3_free(zErrMsg);
    return false;
  }
  _tableStoreMap[tableName].add_row(r);
  LOG_TO_DEBUG("Added Event in table=%s", tableName.c_str());
  return true;
}

std::shared_ptr<ModelInput> UserEventsManager::get_model_input(
    const std::string& modelInputIdentifier, SinglePreprocessorInput* preprocessorInput,
    int preprocessorInputLength) {
  if (_modelInputToTableNameMap.find(modelInputIdentifier) == _modelInputToTableNameMap.end()) {
    LOG_TO_ERROR("No preprocessor for modelInputId=%s", modelInputIdentifier.c_str());
    return nullptr;
  }
  const auto tableName = _modelInputToTableNameMap[modelInputIdentifier];
  if (_tableStoreMap.find(tableName) == _tableStoreMap.end()) {
    LOG_TO_ERROR("TableName=%s not found for modelInputId=%s", tableName.c_str(),
                 modelInputIdentifier.c_str());
    return nullptr;
  }
  return _tableStoreMap[tableName].get_model_input(modelInputIdentifier, preprocessorInput,
                                                   preprocessorInputLength);
}

void from_json(const json& j, TableInfo& tableInfo) {
  j.at("tableName").get_to(tableInfo.name);
  tableInfo.schema = j.at("schema").get<std::map<std::string, std::string>>();
  j.at("expiryInMins").get_to(tableInfo.expiryTimeInMins);
  tableInfo.valid = true;
}

template bool UserEventsManager::create_preprocessor<float>(const std::string&, const json&);
template bool UserEventsManager::create_preprocessor<double>(const std::string&, const json&);
template bool UserEventsManager::create_preprocessor<int64_t>(const std::string&, const json&);
template bool UserEventsManager::create_preprocessor<int32_t>(const std::string&, const json&);