#pragma once
#include <shared_mutex>

#include "CommandCenter.h"
#include "ResourceManagerStructs.h"
class CommandCenter;

struct Feature {
  int dataType = -1;
  char* data = nullptr;
  int byteLength = 0;
  std::string version;
};

class FeatureStore {
  std::shared_mutex _m;
  bool _valid = false;
  Feature _feature;
  uint64_t _expiry = 0;
  bool _isUptoDate = false;

 public:
  const Feature& get_feature() { return _feature; }
  bool get_read_lock(std::shared_lock<std::shared_mutex>& m);
  bool load_feature_store(const FeatureData& featureData);
  bool should_download();

  void set_upto_date() { _isUptoDate = true; }
  const std::string& get_version() { return _feature.version; }
};

class FeatureStoreManager {
  CommandCenter* _commandCenter;
  std::map<std::string, FeatureStore*> _featureMap;

 public:
  FeatureStoreManager(CommandCenter* commandCenter);
  bool get_read_lock(const std::string& featureName, std::shared_lock<std::shared_mutex>& m);
  bool load_feature_store(const std::string& featureName, const FeatureData& featureData);
  bool should_download(const std::string& featureName);
  const Feature& get_feature(const std::string& featureName);
  const std::string& get_version(const std::string& featureName);
  void set_upto_date(const std::string& featureName);
};