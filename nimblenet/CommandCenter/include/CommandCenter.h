#pragma once
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "CommandCenterStructs.h"
#include "ConfigManager.h"
#include "Executor.h"
#include "ExecutorStructs.h"
#include "FeatureStore.h"
#include "ResourceManager.h"
#include "UserEventsManager.h"
#include "util.h"

class ResourceManager;
class ONNXExecutor;
class FeatureStoreManager;

class CommandCenter {
  std::mutex _initMutex;  // ensures only one initialize is ever called
  ResourceManager* _resourceManager = nullptr;
  UserEventsManager* _userEventsManager = nullptr;
  Config _config;
  bool _initializeSuccess = false;
  int _inferenceCount = 0;
  CloudConfig _cloudConfig;
  bool _cloudConfigSet = false;
  Logger _metricsAgent;
  std::chrono::time_point<std::chrono::high_resolution_clock> _lastMetricTime =
      std::chrono::high_resolution_clock::now() -
      std::chrono::seconds(2 * METRICS_COLLECTION_TIME_INTERVAL_IN_SECONDS);
  std::unordered_map<std::string, std::chrono::time_point<std::chrono::high_resolution_clock> >
      _inferenceStartMap;
  std::thread _cmdThread;
#ifdef TORCH_EXECUTOR
  TorchExecutor* _mlExecutor = nullptr;
#elif ONNX_EXECUTOR
  ONNXExecutor* _mlExecutor = nullptr;
#else
  COMPILATION FAIL Please define the executor to use using - DTORCH_EXECUTOR or
      -DONNX_EXECUTOR
#endif
  FeatureStoreManager* _featureStoreManager = nullptr;
  int _cloudConfigFetchRetries = DEFAULT_FETCH_CLOUD_CONFIG_RETRIES;

  int get_inference_count() const { return _inferenceCount; }

  void perform_long_running_tasks();
  void send_metrics(bool initComplete);
  void get_metrics();

 public:
  int initialize(const Config& config);

  int get_inference(const std::string& modelId, const ONNXInferenceRequest& req,
                    ONNXInferenceReturn* ret, const std::string& inferId);
  void exit_inference(const std::string& modelId, const std::string& inferId,
                      bool inferenceSuccess);
  bool get_model_status(const std::string& modelId, ModelStatus* status);

  bool add_user_event(const std::string& eventMapJsonString, const std::string& tableName);

  void set_cloud_config();

  ONNXExecutor* get_mlExecutor() { return _mlExecutor; }

  UserEventsManager* get_userEventsManager() { return _userEventsManager; }

  FeatureStoreManager* get_feature_store_manager() { return _featureStoreManager; }

  const Config& get_config() { return _config; }

  void log_metrics(const char* metricType, const nlohmann::json& metric);

  void write_log(const std::string& log);

  void write_metric(const char* metricType, const char* metricJson);

  void internet_switched_on();
  bool get_inputs_for_inferId(const std::string& inferId, ONNXInferenceReturn* ret);
};