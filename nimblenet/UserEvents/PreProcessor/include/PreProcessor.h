#include <map>
#include <string>
#include <vector>

#include "util.h"

#pragma once
#include "RollingWindow.h"
#include "UserEventsStruct.h"

template <class T>
class RollingWindow;

class BasePreProcessor {
 public:
  int _id;
  virtual void add_event(const std::vector<TableEvent>& allEvents, int newEventIndex) = 0;
  virtual std::string get_group_from_event(const TableEvent& e) = 0;
  virtual std::string get_group_from_row(const std::vector<std::string>& row,
                                         const std::vector<bool>& columnsFilled) = 0;
  virtual std::shared_ptr<ModelInput> get_model_input(const std::vector<std::string>& groups,
                                                      const std::vector<TableEvent>& allEvents) = 0;

  BasePreProcessor(int id) { _id = id; }
};

template <class T>
class PreProcessor : public BasePreProcessor {
  std::vector<int> _groupIds;
  std::vector<int> _columnIds;
  bool _isUseless = false;
  PreProcessorInfo<T> _info;
  std::vector<T> _defaultFeature;
  std::vector<RollingWindow<T>*> _rollingWindows;
  std::map<std::string, std::vector<T>> _groupWiseFeatureMap;

 public:
  std::string get_group_from_row(const std::vector<std::string>& row,
                                 const std::vector<bool>& columnsFilled);
  std::string get_group_from_event(const TableEvent& e) override;
  std::shared_ptr<ModelInput> get_model_input(const std::vector<std::string>& groups,
                                              const std::vector<TableEvent>& allEvents) override;
  PreProcessor(int id, const PreProcessorInfo<T>& info, const std::vector<int>& groupIds,
               const std::vector<int>& columnIds);
  void add_event(const std::vector<TableEvent>& allEvents, int newEventIndex) override;
};
