#include "RollingWindow.h"

template <class T>
class TimeBasedRollingWindow final : public RollingWindow<T> {
  int _oldestIndex;
  float _windowTime = 0;

 public:
  TimeBasedRollingWindow(int preprocessorId, const PreProcessorInfo<T>& info, float windowTime);
  void add_event(const std::vector<TableEvent>& allEvents, int newEventIndex) override;
  void update_window(const std::vector<TableEvent>& allEvents) override;
};