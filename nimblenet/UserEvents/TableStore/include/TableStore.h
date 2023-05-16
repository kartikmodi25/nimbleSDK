#include <map>
#include <vector>

#include "ExecutorStructs.h"
#include "PreProcessor.h"
#include "UserEventsStruct.h"
class BasePreProcessor;

class TableStore {
  std::string _tableName;
  std::vector<TableEvent> _allEvents;
  std::vector<BasePreProcessor*> _preprocessors;
  std::map<std::string, int> _modelInputPreprocessorIdMap;

  std::map<std::string, int> _columnToIdMap;
  std::vector<std::string> _columns;
  TableInfo _tableInfo;
  bool _isReadyToBeFilled = false;

 public:
  template <class T>
  bool create_preprocessor(const std::string& modelInputIdentifier,
                           const PreProcessorInfo<T>& preprocessorInfo);
  bool check_columns_of_row(const TableRow& r);
  void add_row(const TableRow& r);
  std::shared_ptr<ModelInput> get_model_input(const std::string& modelInputIdentifier,
                                              SinglePreprocessorInput* preprocessorInput,
                                              int preprocessorInputLength);
  TableStore(const TableInfo&);

  TableStore(){};

  void set_to_fill_in_memory() { _isReadyToBeFilled = true; }

  bool is_table_ready_to_be_filled() { return _isReadyToBeFilled; }
};