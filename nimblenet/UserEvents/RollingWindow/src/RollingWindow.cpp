#include "RollingWindow.h"

#include "time.h"
using namespace std;

template <class T>
bool RollingWindow<T>::create_aggregate_columns_for_group(const std::string& group,
                                                          const std::vector<int>& columnIds,
                                                          std::vector<T>& totalFeatureVector,
                                                          int rollingWindowFeatureStartIndex) {
  _groupWiseAggregatedColumnMap[group] = std::vector<AggregateColumn<T>*>();
  for (int i = 0; i < _preprocessorInfo.columnsToAggregate.size(); i++) {
    if (_preprocessorInfo.aggregateOperators[i] == "Sum")
      _groupWiseAggregatedColumnMap[group].push_back(
          new SumColumn<T>(_preprocessorId, columnIds[i], group,
                           &totalFeatureVector[rollingWindowFeatureStartIndex + i]));
    else if (_preprocessorInfo.aggregateOperators[i] == "Count")
      _groupWiseAggregatedColumnMap[group].push_back(
          new CountColumn<T>(_preprocessorId, columnIds[i], group,
                             &totalFeatureVector[rollingWindowFeatureStartIndex + i]));
    else if (_preprocessorInfo.aggregateOperators[i] == "Min")
      _groupWiseAggregatedColumnMap[group].push_back(
          new MinColumn<T>(_preprocessorId, columnIds[i], group,
                           &totalFeatureVector[rollingWindowFeatureStartIndex + i]));
    else if (_preprocessorInfo.aggregateOperators[i] == "Max")
      _groupWiseAggregatedColumnMap[group].push_back(
          new MaxColumn<T>(_preprocessorId, columnIds[i], group,
                           &totalFeatureVector[rollingWindowFeatureStartIndex + i]));
    else if (_preprocessorInfo.aggregateOperators[i] == "Avg")
      _groupWiseAggregatedColumnMap[group].push_back(
          new AverageColumn<T>(_preprocessorId, columnIds[i], group,
                               &totalFeatureVector[rollingWindowFeatureStartIndex + i]));
    else {
      LOG_TO_ERROR("%s",
                   "No Operators found for the following column, Operators can be Count, Min, Max, "
                   "Sum, Avg");
      return false;
    }
  }
  return true;
}
template class RollingWindow<float>;
template class RollingWindow<double>;
template class RollingWindow<int32_t>;
template class RollingWindow<int64_t>;
