#include "FeatureStore.h"

#include <time.h>

FeatureStoreManager::FeatureStoreManager(CommandCenter* commandCenter) {
  _commandCenter = commandCenter;
  for (const auto featureName : _commandCenter->get_config().featureStores) {
    _featureMap[featureName] = new FeatureStore;
  }
}

bool FeatureStore::get_read_lock(std::shared_lock<std::shared_mutex>& m) {
  if (_valid) {
    m = std::move(std::shared_lock<std::shared_mutex>(_m));
    return true;
  }
  return false;
}

bool FeatureStoreManager::get_read_lock(const std::string& featureName,
                                        std::shared_lock<std::shared_mutex>& m) {
  if (_featureMap.find(featureName) != _featureMap.end()) {
    bool ret = _featureMap[featureName]->get_read_lock(m);
    if (!ret) LOG_TO_ERROR("featureName=%s, is not loaded in memory yet", featureName.c_str());
    return ret;
  } else {
    LOG_TO_ERROR("get_read_lock featureName=%s not given in config", featureName.c_str());
    return false;
  }
}

const Feature& FeatureStoreManager::get_feature(const std::string& featureName) {
  if (_featureMap.find(featureName) != _featureMap.end()) {
    return _featureMap[featureName]->get_feature();
  } else {
    // This should never get called, incorrect use. Should call get_read_lock before calling
    // get_data
    LOG_TO_FATAL("get_feature failed for featureName=%s", featureName.c_str());
    return Feature();
  }
}

bool FeatureStore::load_feature_store(const FeatureData& featureData) {
  char* newBufferForFeature = new char[featureData.featureLength];
  memcpy(newBufferForFeature, featureData.feature, featureData.featureLength);
  char* bufferToDelete = _feature.data;
  // locked
  std::unique_lock<std::shared_mutex> writeLock(_m);
  _feature.data = newBufferForFeature;
  _feature.dataType = featureData.dataType;
  _feature.byteLength = featureData.featureLength;
  _feature.version = featureData.version;
  _valid = true;
  _expiry = featureData.expiry;
  writeLock.unlock();
  // unlocked
  if (bufferToDelete) delete bufferToDelete;
  return true;
}

bool FeatureStoreManager::load_feature_store(const std::string& featureName,
                                             const FeatureData& featureData) {
  if (_featureMap.find(featureName) != _featureMap.end()) {
    return _featureMap[featureName]->load_feature_store(featureData);
  } else {
    LOG_TO_ERROR("load_feature_store failed No feature found of the name=%s", featureName.c_str());
    return false;
  }
}

const std::string& FeatureStoreManager::get_version(const std::string& featureName) {
  if (_featureMap.find(featureName) != _featureMap.end()) {
    return _featureMap[featureName]->get_version();
  } else {
    LOG_TO_ERROR("get_version failed No feature found of the name=%s", featureName.c_str());
    return "";
  }
}

bool FeatureStore::should_download() {
  if (!_isUptoDate) return true;
  uint64_t seconds = time(NULL);
  return _expiry < seconds;
}

bool FeatureStoreManager::should_download(const std::string& featureName) {
  if (_featureMap.find(featureName) != _featureMap.end()) {
    return _featureMap[featureName]->should_download();
  } else {
    LOG_TO_ERROR("should_download failed No feature found of the name=%s", featureName.c_str());
    return false;
  }
}

void FeatureStoreManager::set_upto_date(const std::string& featureName) {
  if (_featureMap.find(featureName) != _featureMap.end()) {
    _featureMap[featureName]->set_upto_date();
  } else {
    LOG_TO_ERROR("set_upto_date No feature found of the name=%s", featureName.c_str());
  }
}