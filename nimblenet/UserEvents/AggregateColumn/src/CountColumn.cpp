#include "CountColumn.h"

#include "util.h"
using namespace std;

template <class T>
void CountColumn<T>::add_event(const std::vector<TableEvent>& allEvents, int newEventIndex) {
  const auto& event = allEvents[newEventIndex];
  const auto eventGroup = event.groups[this->_preprocessorId];
  if (eventGroup != this->_group) {
    LOG_TO_ERROR("CountColumn: add_event event.group=%s not same as column.group=%s",
                 eventGroup.c_str(), this->_group.c_str());
    return;
  }
  this->_totalCount++;
  if (_oldestIndex == -1) {
    _oldestIndex = newEventIndex;
    *this->_storeValue = 1;
    return;
  }
  *this->_storeValue = (*this->_storeValue) + 1;
}

template <class T>
void CountColumn<T>::remove_events(const std::vector<TableEvent>& allEvents, int oldestValidIndex) {
  for (int i = _oldestIndex; i < oldestValidIndex; i++) {
    const auto eventGroup = allEvents[i].groups[this->_preprocessorId];
    if (eventGroup == this->_group) {
      *this->_storeValue = (*this->_storeValue) - 1;
      this->_totalCount--;
    }
  }
  if (this->_totalCount == 0) {
    *this->_storeValue = this->_defaultValue;
    _oldestIndex = -1;
    return;
  }
  _oldestIndex = oldestValidIndex;
}
template class CountColumn<float>;
template class CountColumn<double>;
template class CountColumn<int32_t>;
template class CountColumn<int64_t>;
