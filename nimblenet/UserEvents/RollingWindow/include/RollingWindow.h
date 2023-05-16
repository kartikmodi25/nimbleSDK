#pragma once
#include <map>
#include <vector>

#include "AggregateColumn.h"
#include "UserEventsStruct.h"

template <class T>
class AggregateColumn;

template <typename T>
class RollingWindow {
 public:
  int _preprocessorId;
  PreProcessorInfo<T> _preprocessorInfo;
  std::map<std::string, std::vector<AggregateColumn<T>*>> _groupWiseAggregatedColumnMap;
  bool create_aggregate_columns_for_group(const std::string& group,
                                          const std::vector<int>& columnIds,
                                          std::vector<T>& totalFeatureVector,
                                          int rollingWindowFeatureStartIndex);
  virtual void add_event(const std::vector<TableEvent>& allEvents, int newEventIndex) = 0;
  virtual void update_window(const std::vector<TableEvent>& allEvents) = 0;

  RollingWindow(int preprocessorId, const PreProcessorInfo<T>& info) {
    _preprocessorInfo = info;
    _preprocessorId = preprocessorId;
  }
};
