#include "MaxColumn.h"

#include "util.h"
using namespace std;

template <class T>
void MaxColumn<T>::add_event(const std::vector<TableEvent>& allEvents, int newEventIndex) {
  const auto& event = allEvents[newEventIndex];
  const auto eventGroup = event.groups[this->_preprocessorId];
  if (eventGroup != this->_group) {
    LOG_TO_ERROR("MaxColumn: add_event event.group=%s not same as column.group=%s",
                 eventGroup.c_str(), this->_group.c_str());
    return;
  }
  this->_totalCount++;
  if (_oldestIndex == -1) {
    _oldestIndex = newEventIndex;
    *this->_storeValue = GetAs<T>(event.row[this->_columnId]);
    return;
  }

  *this->_storeValue = max(*this->_storeValue, GetAs<T>(event.row[this->_columnId]));
}

template <class T>
void MaxColumn<T>::remove_events(const std::vector<TableEvent>& allEvents, int oldestValidIndex) {
  bool isMaxChanged = false;
  for (int i = _oldestIndex; i < allEvents.size(); i++) {
    if (!isMaxChanged && (i >= oldestValidIndex)) {
      break;
    }
    const auto eventGroup = allEvents[i].groups[this->_preprocessorId];
    if (eventGroup == this->_group) {
      T val = GetAs<T>(allEvents[i].row[this->_columnId]);
      if (isMaxChanged) {
        if (this->_totalCount == 0) {
          *this->_storeValue = val;
          _oldestIndex = i;
        } else
          *this->_storeValue = max(*this->_storeValue, val);
        this->_totalCount++;
      } else if (val == *this->_storeValue) {
        isMaxChanged = true;
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
template class MaxColumn<float>;
template class MaxColumn<double>;
template class MaxColumn<int32_t>;
template class MaxColumn<int64_t>;
