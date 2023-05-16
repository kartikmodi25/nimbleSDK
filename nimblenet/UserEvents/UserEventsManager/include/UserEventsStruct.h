#include <map>
#include <string>
#include <vector>

#include "util.h"
#pragma once

struct ModelInput {
  int length = 0;
  void* data = nullptr;

  ModelInput(void* d, int l) {
    data = d;
    length = l;
  }

  ~ModelInput() { free(data); }
};

struct TableInfo {
  bool valid = false;
  std::string name;
  std::map<std::string, std::string> schema;
  int64_t expiryTimeInMins;
};

template <class T>
struct PreProcessorInfo {
  std::vector<float> rollingWindowsInSecs;
  std::vector<std::string> columnsToAggregate;
  std::vector<std::string> aggregateOperators;
  std::vector<std::string> groupColumns;
  std::vector<T> defaultVector;
  std::string tableName;
  bool valid = false;
};

template <class T>
void from_json(const json& j, PreProcessorInfo<T>& preProcessorInfo);

struct TableEvent {
  std::vector<std::string> groups;
  int64_t timestamp;
  std::vector<std::string> row;
};

struct TableRow {
  int64_t timestamp;
  std::map<std::string, std::string> row;
};