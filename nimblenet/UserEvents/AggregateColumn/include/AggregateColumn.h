#pragma once
#include <string>
#include <vector>

#include "UserEventsStruct.h"
using namespace std;

template <class T>
class AggregateColumn {
 public:
  T* _storeValue = nullptr;
  int _columnId;
  std::string _group;
  int _preprocessorId;
  T _defaultValue;
  int _totalCount = 0;

  AggregateColumn(int preprocessorId, int columnId, const std::string& group, T* storePtr) {
    _preprocessorId = preprocessorId;
    _columnId = columnId;
    _group = group;
    _storeValue = storePtr;
    _defaultValue = *storePtr;
  }

  virtual void add_event(const std::vector<TableEvent>& allEvents, int newEventIndex) = 0;
  virtual void remove_events(const std::vector<TableEvent>& allEvents, int oldestValidIndex) = 0;
};

#include "AverageColumn.h"
#include "CountColumn.h"
#include "MaxColumn.h"
#include "MinColumn.h"
#include "SumColumn.h"

template <typename T>
T GetAs(const std::string& s) {
  std::stringstream ss{s};
  T t;
  if (!(ss >> t)) LOG_TO_FATAL("%s cannot be converted to %s", s.c_str(), typeid(T).name());
  return t;
}
