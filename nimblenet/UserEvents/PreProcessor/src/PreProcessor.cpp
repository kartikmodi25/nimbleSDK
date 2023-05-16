#include "PreProcessor.h"

#include "TimeBasedRollingWindow.h"
using namespace std;

template <class T>
PreProcessor<T>::PreProcessor(int id, const PreProcessorInfo<T>& info,
                              const std::vector<int>& groupIds, const std::vector<int>& columnIds)
    : BasePreProcessor(id) {
  _info = info;
  _groupIds = groupIds;
  _columnIds = columnIds;
  for (auto rWindow : _info.rollingWindowsInSecs) {
    _rollingWindows.push_back(new TimeBasedRollingWindow<T>(id, _info, rWindow));
  }
  _defaultFeature =
      std::vector<T>(_info.rollingWindowsInSecs.size() * _info.columnsToAggregate.size());
  for (int i = 0; i < _defaultFeature.size(); i++) {
    _defaultFeature[i] = _info.defaultVector[i % _info.columnsToAggregate.size()];
  }
}

template <class T>
string PreProcessor<T>::get_group_from_event(const TableEvent& e) {
  string group = "";
  for (auto groupId : _groupIds) {
    // magic way to create group
    group += e.row[groupId] + "+";
  }
  return group;
}

template <class T>
string PreProcessor<T>::get_group_from_row(const std::vector<std::string>& row,
                                           const std::vector<bool>& columnsFilled) {
  string group = "";
  for (auto groupId : _groupIds) {
    if (!columnsFilled[groupId]) {
      LOG_TO_ERROR("Could not form group for entity, groupId=%d is missing", groupId);
      return "";
    }
    // magic way to create group
    group += row[groupId] + "+";
  }
  return group;
}

template <class T>
std::shared_ptr<ModelInput> PreProcessor<T>::get_model_input(
    const std::vector<std::string>& groups, const std::vector<TableEvent>& allEvents) {
  if (_isUseless) {
    LOG_TO_ERROR("%s", "Preprocessor get_model_input failed");
    return nullptr;
  }
  for (const auto& rollingwindow : _rollingWindows) {
    rollingwindow->update_window(allEvents);
  }
  T* inputData = (T*)malloc(groups.size() * _defaultFeature.size() * sizeof(T));
  for (int i = 0; i < groups.size(); i++) {
    string group = groups[i];
    if (_groupWiseFeatureMap.find(group) == _groupWiseFeatureMap.end()) {
      memcpy(&inputData[i * _defaultFeature.size()], &_defaultFeature[0],
             _defaultFeature.size() * sizeof(T));
    } else {
      memcpy(&inputData[i * _defaultFeature.size()], &_groupWiseFeatureMap[group][0],
             _defaultFeature.size() * sizeof(T));
    }
  }
  int length = groups.size() * _defaultFeature.size();
  return std::make_shared<ModelInput>((void*)inputData, length);
}

template <class T>
void PreProcessor<T>::add_event(const std::vector<TableEvent>& allEvents, int newEventIndex) {
  const auto& e = allEvents[newEventIndex];
  const auto& group = e.groups[_id];
  if (_groupWiseFeatureMap.find(group) == _groupWiseFeatureMap.end()) {
    _groupWiseFeatureMap[group] = std::vector<T>(_defaultFeature.size());
    _groupWiseFeatureMap[group] = _defaultFeature;
    for (int i = 0; i < _rollingWindows.size(); i++) {
      bool isSuccess = _rollingWindows[i]->create_aggregate_columns_for_group(
          group, _columnIds, _groupWiseFeatureMap[group], i * _info.columnsToAggregate.size());
      if (!isSuccess) _isUseless = true;
    }
  }
  for (auto rollingWindow : _rollingWindows) {
    rollingWindow->add_event(allEvents, newEventIndex);
  }
}

template class PreProcessor<float>;
template class PreProcessor<double>;
template class PreProcessor<int64_t>;
template class PreProcessor<int32_t>;
