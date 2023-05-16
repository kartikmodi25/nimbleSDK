#pragma once
#include "AggregateColumn.h"

template <class T>
class SumColumn final : public AggregateColumn<T> {
  int _oldestIndex = -1;

 public:
  SumColumn(int preprocessorId, int columnId, const std::string& group, T* storePtr)
      : AggregateColumn<T>(preprocessorId, columnId, group, storePtr){};

  void add_event(const std::vector<TableEvent>& allEvents, int newEventIndex) override;
  void remove_events(const std::vector<TableEvent>& allEvents, int oldestValidIndex) override;
};