#include "SumColumn.h"

#include "util.h"
using namespace std;

template <class T>
void SumColumn<T>::add_event(const std::vector<TableEvent>& allEvents, int newEventIndex) {
  const auto& event = allEvents[newEventIndex];
  const auto eventGroup = event.groups[this->_preprocessorId];
  if (eventGroup != this->_group) {
    LOG_TO_ERROR("SumColumn: add_event event.group=%s not same as column.group=%s",
                 eventGroup.c_str(), this->_group.c_str());
    return;
  }
  this->_totalCount++;
  if (_oldestIndex == -1) {
    _oldestIndex = newEventIndex;
    *this->_storeValue = GetAs<T>(event.row[this->_columnId]);
    return;
  }
  *this->_storeValue = (*this->_storeValue) + GetAs<T>(event.row[this->_columnId]);
}

template <class T>
void SumColumn<T>::remove_events(const std::vector<TableEvent>& allEvents, int oldestValidIndex) {
  for (int i = _oldestIndex; i < oldestValidIndex; i++) {
    const auto eventGroup = allEvents[i].groups[this->_preprocessorId];
    if (eventGroup == this->_group) {
      this->_totalCount--;
      *this->_storeValue = (*this->_storeValue) - GetAs<T>(allEvents[i].row[this->_columnId]);
    }
  }
  if (this->_totalCount == 0) {
    *this->_storeValue = this->_defaultValue;
    _oldestIndex = -1;
    return;
  }
  _oldestIndex = oldestValidIndex;
}
template class SumColumn<float>;
template class SumColumn<double>;
template class SumColumn<int32_t>;
template class SumColumn<int64_t>;
