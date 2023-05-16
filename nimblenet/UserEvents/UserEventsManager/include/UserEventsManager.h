#include <sqlite3.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "TableStore.h"
#include "UserEventsStruct.h"
#include "util.h"

void from_json(const json& j, TableInfo& tableInfo);

class UserEventsManager {
  sqlite3* _db = nullptr;
  std::map<std::string, TableStore> _tableStoreMap;
  std::map<std::string, std::string> _modelInputToTableNameMap;
  bool create_table(const json& tableInfoJson);
  bool delete_old_rows_from_table_in_db(const TableInfo& tableInfo);
  bool create_table_in_db(const TableInfo& tableInfo);
  std::string get_existing_create_table_command(const std::string& tableName);
  bool _debugMode = false;

 public:
  UserEventsManager(const std::vector<json>& tableInfos, bool debug = false);
  std::shared_ptr<ModelInput> get_model_input(const std::string& modelInputIdentifier,
                                              SinglePreprocessorInput* preprocessorInput,
                                              int preprocessorInputLength);
  template <class T>
  bool create_preprocessor(const std::string& modelInputIdentifier, const json& preprocessorJson);
  bool add_event(const std::string& eventMapJsonString, const std::string& tableName);
};
