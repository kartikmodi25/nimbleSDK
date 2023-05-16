#include "AverageColumn.h"

#include "util.h"
using namespace std;

template <class T>
void AverageColumn<T>::add_event(const std::vector<TableEvent>& allEvents, int newEventIndex) {
  const auto& event = allEvents[newEventIndex];
  const auto eventGroup = event.groups[this->_preprocessorId];
  if (eventGroup != this->_group) {
    LOG_TO_ERROR("AverageColumn: add_event event.group=%s not same as column.group=%s",
                 eventGroup.c_str(), this->_group.c_str());
    return;
  }
  this->_totalCount++;
  if (_oldestIndex == -1) {
    _oldestIndex = newEventIndex;
  }
  _sum += GetAs<T>(event.row[this->_columnId]);
  *this->_storeValue = _sum / this->_totalCount;
}

template <class T>
void AverageColumn<T>::remove_events(const std::vector<TableEvent>& allEvents,
                                     int oldestValidIndex) {
  for (int i = _oldestIndex; i < oldestValidIndex; i++) {
    const auto eventGroup = allEvents[i].groups[this->_preprocessorId];
    if (eventGroup == this->_group) {
      this->_totalCount--;
      _sum -= GetAs<T>(allEvents[i].row[this->_columnId]);
    }
  }
  if (this->_totalCount == 0) {
    _sum = 0;
    *this->_storeValue = this->_defaultValue;
    _oldestIndex = -1;
    return;
  }
  *this->_storeValue = _sum / this->_totalCount;
  _oldestIndex = oldestValidIndex;
}
template class AverageColumn<float>;
template class AverageColumn<double>;
template class AverageColumn<int32_t>;
template class AverageColumn<int64_t>;
