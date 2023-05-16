#include "TimeBasedRollingWindow.h"

template <class T>
TimeBasedRollingWindow<T>::TimeBasedRollingWindow(int preprocessorId,
                                                  const PreProcessorInfo<T>& info, float windowTime)
    : RollingWindow<T>(preprocessorId, info) {
  _windowTime = windowTime;
  _oldestIndex = -1;
}

template <typename T>
void TimeBasedRollingWindow<T>::add_event(const std::vector<TableEvent>& allEvents,
                                          int newEventIndex) {
  const auto& event = allEvents[newEventIndex];
  if (time(NULL) - event.timestamp > _windowTime) {
    return;
  }
  if (_oldestIndex == -1) {
    _oldestIndex = newEventIndex;
  }
  for (auto aggregatedColumn :
       this->_groupWiseAggregatedColumnMap[event.groups[this->_preprocessorId]]) {
    aggregatedColumn->add_event(allEvents, newEventIndex);
  }
}

template <class T>
void TimeBasedRollingWindow<T>::update_window(const std::vector<TableEvent>& allEvents) {
  auto currTime = time(NULL);
  for (int i = _oldestIndex; i < allEvents.size(); i++) {
    if (time(NULL) - allEvents[i].timestamp > _windowTime) {
      _oldestIndex++;
    } else {
      // none of the already existing event in the rolling window have expired
      return;
    }
  }
  for (auto it = this->_groupWiseAggregatedColumnMap.begin();
       it != this->_groupWiseAggregatedColumnMap.end(); it++) {
    for (auto aggregatedColumn : it->second) {
      aggregatedColumn->remove_events(allEvents, _oldestIndex);
    }
  }
}
template class TimeBasedRollingWindow<float>;
template class TimeBasedRollingWindow<double>;
template class TimeBasedRollingWindow<int32_t>;
template class TimeBasedRollingWindow<int64_t>;
