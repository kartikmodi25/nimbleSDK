#include "MinColumn.h"

#include "util.h"
using namespace std;

template <class T>
void MinColumn<T>::add_event(const std::vector<TableEvent>& allEvents, int newEventIndex) {
  const auto& event = allEvents[newEventIndex];
  const auto eventGroup = event.groups[this->_preprocessorId];
  if (eventGroup != this->_group) {
    LOG_TO_ERROR("MinColumn: add_event event.group=%s not same as column.group=%s",
                 eventGroup.c_str(), this->_group.c_str());
    return;
  }
  this->_totalCount++;
  if (_oldestIndex == -1) {
    _oldestIndex = newEventIndex;
    *this->_storeValue = GetAs<T>(event.row[this->_columnId]);
    return;
  }

  *this->_storeValue = min(*this->_storeValue, GetAs<T>(event.row[this->_columnId]));
}

template <class T>
void MinColumn<T>::remove_events(const std::vector<TableEvent>& allEvents, int oldestValidIndex) {
  bool isMinChanged = false;
  for (int i = _oldestIndex; i < allEvents.size(); i++) {
    if (!isMinChanged && (i >= oldestValidIndex)) {
      break;
    }
    const auto eventGroup = allEvents[i].groups[this->_preprocessorId];
    if (eventGroup == this->_group) {
      T val = GetAs<T>(allEvents[i].row[this->_columnId]);
      if (isMinChanged) {
        if (this->_totalCount == 0) {
          *this->_storeValue = val;
          _oldestIndex = i;
        } else
          *this->_storeValue = min(*this->_storeValue, val);
        this->_totalCount++;
      } else if (val == *this->_storeValue) {
        isMinChanged = true;
        this->_totalCount = 0;
        *this->_storeValue = this->_defaultValue;
        i = oldestValidIndex - 1;
      } else {
        this->_totalCount--;
      }
    }
  }
  if (this->_totalCount == 0) {
    *this->_storeValue = this->_defaultValue;
    _oldestIndex = -1;
    return;
  }
  _oldestIndex = oldestValidIndex;
}
template class MinColumn<float>;
template class MinColumn<double>;
template class MinColumn<int32_t>;
template class MinColumn<int64_t>;
