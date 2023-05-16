#include "TableStore.h"
using namespace std;

bool TableStore::check_columns_of_row(const TableRow& r) {
  for (auto it = r.row.begin(); it != r.row.end(); it++) {
    if (_columnToIdMap.find(it->first) == _columnToIdMap.end()) {
      LOG_TO_ERROR("Column=%s is not present in table=%s, won't be able to add this event",
                   it->first.c_str(), _tableName.c_str());
      return false;
    }
  }

  // this check might have to be removed for derived columns
  for (auto it = _columnToIdMap.begin(); it != _columnToIdMap.end(); it++) {
    if (r.row.find(it->first) == r.row.end()) {
      LOG_TO_ERROR(
          "Column=%s is present in table=%s, but not in event. won't be able to add this event",
          it->first.c_str(), _tableName.c_str());
      return false;
    }
  }
  return true;
}

void TableStore::add_row(const TableRow& r) {
  if (!_isReadyToBeFilled) {
    // this event will be added when we fetch the entire data from db
    return;
  }
  int newIndex = _allEvents.size();
  TableEvent e;
  e.timestamp = r.timestamp;
  for (auto it = r.row.begin(); it != r.row.end(); it++) {
    if (_columnToIdMap.find(it->first) == _columnToIdMap.end()) {
      LOG_TO_ERROR("Column=%s not found in table=%s", it->first.c_str(), _tableName.c_str());
      return;
    }
    e.row.push_back(it->second);
  }
  _allEvents.push_back(e);
  for (auto preprocessor : _preprocessors) {
    _allEvents[newIndex].groups.push_back(preprocessor->get_group_from_event(e));
    preprocessor->add_event(_allEvents, _allEvents.size() - 1);
  }
}

template <class T>
bool TableStore::create_preprocessor(const std::string& modelInputIdentifier,
                                     const PreProcessorInfo<T>& info) {
  BasePreProcessor* bpreprocessor = nullptr;
  int newPreprocessorId = _preprocessors.size();
  std::vector<int> groupIds;
  std::vector<int> columnIds;
  for (int i = 0; i < info.groupColumns.size(); i++) {
    if (_columnToIdMap.find(info.groupColumns[i]) == _columnToIdMap.end()) {
      LOG_TO_ERROR("Column %s(to group by) not present in table %s", info.groupColumns[i].c_str(),
                   info.tableName.c_str());
      return false;
    }
    groupIds.push_back(_columnToIdMap[info.groupColumns[i]]);
  }
  for (int i = 0; i < info.columnsToAggregate.size(); i++) {
    string columnName = info.columnsToAggregate[i];
    if (_columnToIdMap.find(columnName) == _columnToIdMap.end()) {
      LOG_TO_ERROR("Column %s(to aggregate on) not present in table %s", columnName.c_str(),
                   info.tableName.c_str());
      return false;
    }
    string aggregateOperator = info.aggregateOperators[i];
    string dataTypeOfColumn = _tableInfo.schema[columnName];
    transform(dataTypeOfColumn.begin(), dataTypeOfColumn.end(), dataTypeOfColumn.begin(),
              ::tolower);
    if ((aggregateOperator != "Count") && (dataTypeOfColumn.find("varchar") != std::string::npos)) {
      LOG_TO_ERROR("Column=%s cannot be aggregated using operator=%s", columnName.c_str(),
                   aggregateOperator.c_str());
      return false;
    }

    columnIds.push_back(_columnToIdMap[info.columnsToAggregate[i]]);
  }
  try {
    bpreprocessor = new PreProcessor<T>(newPreprocessorId, info, groupIds, columnIds);
  } catch (...) {
    LOG_TO_ERROR("PreProcessor could not be created for modelInputIdentifier=%s",
                 modelInputIdentifier.c_str());
    return false;
  }

  // preprocessor object created, adding events to preprocessor from allEvents
  for (int i = 0; i < _allEvents.size(); i++) {
    _allEvents[i].groups.push_back(bpreprocessor->get_group_from_event(_allEvents[i]));
    bpreprocessor->add_event(_allEvents, i);
  }
  _modelInputPreprocessorIdMap[modelInputIdentifier] = newPreprocessorId;
  _preprocessors.push_back(bpreprocessor);
  return true;
}

std::shared_ptr<ModelInput> TableStore::get_model_input(const std::string& modelInputIdentifier,
                                                        SinglePreprocessorInput* preprocessorInput,
                                                        int preprocessorInputLength) {
  if (_modelInputPreprocessorIdMap.find(modelInputIdentifier) ==
      _modelInputPreprocessorIdMap.end()) {
    LOG_TO_ERROR("modelInput=%s, has no preprocessor created", modelInputIdentifier.c_str());
    return nullptr;
  }
  int preprocessorIndex = _modelInputPreprocessorIdMap[modelInputIdentifier];
  std::vector<std::string> groups;
  for (int i = 0; i < preprocessorInputLength; i++) {
    std::vector<std::string> row(_columns.size());
    vector<bool> columnFilled(_columns.size(), false);
    for (int j = 0; j < preprocessorInput[i].keysLength; j++) {
      auto key = preprocessorInput[i].keys[j];
      auto value = preprocessorInput[i].values[j];
      if (_columnToIdMap.find(key) == _columnToIdMap.end()) {
        continue;
      }
      int columnIndex = _columnToIdMap[key];
      columnFilled[columnIndex] = true;
      row[columnIndex] = value;
    }
    auto group = _preprocessors[preprocessorIndex]->get_group_from_row(row, columnFilled);
    if (group == "") {
      LOG_TO_ERROR("Could not find group for preprocessor of input=%s",
                   modelInputIdentifier.c_str());
      return nullptr;
    }
    groups.push_back(group);
  }
  return _preprocessors[preprocessorIndex]->get_model_input(groups, _allEvents);
}

TableStore::TableStore(const TableInfo& tableInfo) {
  _tableInfo = tableInfo;
  _tableName = tableInfo.name;
  int id = 0;
  for (const auto& it : _tableInfo.schema) {
    _columnToIdMap[it.first] = id++;
    _columns.push_back(it.first);
  }
}

template bool TableStore::create_preprocessor<float>(const std::string&,
                                                     const PreProcessorInfo<float>&);
template bool TableStore::create_preprocessor<double>(const std::string&,
                                                      const PreProcessorInfo<double>&);
template bool TableStore::create_preprocessor<int64_t>(const std::string&,
                                                       const PreProcessorInfo<int64_t>&);
template bool TableStore::create_preprocessor<int32_t>(const std::string&,
                                                       const PreProcessorInfo<int32_t>&);