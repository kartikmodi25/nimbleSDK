/* This is the command script from where inference
would be process with set of functionality. */

#include "CommandCenter.h"

#include <pthread.h>
#include <unistd.h>

#include <fstream>
#include <map>

#include "ConfigManager.h"
#include "NativeInterface.h"
#include "ServerAPI.h"
#include "util.h"

using namespace std;

int CommandCenter::initialize(const Config& config) {
  if (!_initMutex.try_lock()) {
    LOG_TO_ERROR("%s",
                 "Initialization is already in progress, might be called from different thread");
    return 0;
  }
  std::lock_guard<std::mutex> guard(_initMutex, std::adopt_lock);
  if (_initializeSuccess) {
    LOG_TO_ERROR("%s", "NimbleNet is already initialized");
    return 1;
  }
  LOG_TO_INFO("%s", "Initializing NimbleNet");

  _config = std::move(config);
  if (!_config.valid) {
    LOG_TO_ERROR("%s", "Config Invalid, Initialize NimbleNet Failed");
    return 0;
  }
  _metricsAgent.init_logger(NativeInterface::HOMEDIR + "/metrics");
  _metricsAgent.update_device_details(_config.deviceId, _config.clientId);
  _resourceManager = new ResourceManager(this);
  _mlExecutor = new ONNXExecutor(this);
  _featureStoreManager = new FeatureStoreManager(this);
  _userEventsManager = new UserEventsManager(_config.tableInfos, _config.debug);

  for (auto modelId : _config.modelIds) {
    _resourceManager->load_plan_from_device(modelId);
  }

  for (auto featureName : _config.featureStores) {
    _resourceManager->load_feature_from_device(featureName);
  }

  sched_param sch_params;
  sch_params.sched_priority = 1;
  _cmdThread = thread(&CommandCenter::perform_long_running_tasks, this);
  pthread_setschedparam(_cmdThread.native_handle(), SCHED_RR, &sch_params);
  _cmdThread.detach();
  _initializeSuccess = true;
  LOG_TO_INFO("%s", "Init NimbleNet succeeded.");
  return 1;
}

int CommandCenter::get_inference(const std::string& modelId, const ONNXInferenceRequest& req,
                                 ONNXInferenceReturn* ret, const std::string& inferId) {
  /* function return inference with user request, logs the inference count,
  and return the inference as per request-reponse schema */
  if (!_initializeSuccess) {
    return TERMINAL_ERROR;
  }
  auto start = std::chrono::high_resolution_clock::now();
  _inferenceStartMap[inferId] = start;
  _inferenceCount++;
  int status = _mlExecutor->get_inference(inferId, modelId, req, ret);
  if (status != SUCCESS) exit_inference(modelId, inferId, false);
  return status;
}

void CommandCenter::exit_inference(const std::string& modelId, const std::string& inferId,
                                   bool inferenceSuccess) {
  if (!_initializeSuccess) {
    return;
  }
  auto stop = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(stop - _inferenceStartMap[inferId]);
  // If model gets updated between get_inference and exit_inference, the modelVersion corresponding
  // to the inference may be wrong and the logs can be incorrect.
  std::string modelVersion = _mlExecutor->get_plan_version(modelId);

  nlohmann::json infermetric;
  infermetric["inferenceId"] = inferId;
  infermetric["inferenceCount"] = get_inference_count();
  infermetric["uTime"] = long(duration.count());
  infermetric["modelId"] = modelId;
  infermetric["modelVersion"] = modelVersion;
  infermetric["success"] = inferenceSuccess ? "true" : "false";
  infermetric["source"] = "cpp";
  _metricsAgent.LOGMETRICS(INFERENCEMETRIC, infermetric.dump().c_str());
  LOG_TO_DEBUG(
      "Id=%s ::: Inference= %d ::: Time= %ld microseconds ::: ModelId=%s ::: Version:%s ::: "
      "Success=%s ::: Source=cpp",
      inferId.c_str(), get_inference_count(), long(duration.count()), modelId.c_str(),
      modelVersion.c_str(), inferenceSuccess ? "true" : "false");
}

void CommandCenter::perform_long_running_tasks() {
  LOG_TO_DEBUG("%s", "Initiating the long running tasks.");
  bool initComplete = false;

  while (1) {
    auto start = std::chrono::high_resolution_clock::now();
    ServerAPI::init(_config);
    set_cloud_config();

    for (auto modelId : _config.modelIds) {
      _resourceManager->update_plan_if_required(modelId);
    }
    for (auto featureName : _config.featureStores) {
      _resourceManager->update_feature_if_required(featureName);
    }
    get_metrics();
    send_metrics(initComplete);
    logger.send_pending_logs(initComplete);

    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    long long timeTakenInMicros =
        std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    auto timeToSleep =
        max((long long)0, (long long)DEFAULT_THREAD_SLEEP_TIME_MICROSECONDS - timeTakenInMicros);
    usleep(timeToSleep);  // configure this
    initComplete = true;
  }
}

void CommandCenter::internet_switched_on() {
  if (!_initializeSuccess) {
    return;
  }
  for (auto modelId : _config.modelIds) {
    if (_mlExecutor) _mlExecutor->reset_model_retries(modelId);
  }
  ServerAPI::reset_register_retries();
  _cloudConfigFetchRetries = DEFAULT_FETCH_CLOUD_CONFIG_RETRIES;
}

void CommandCenter::get_metrics() {
  auto elapsed = std::chrono::high_resolution_clock::now() - _lastMetricTime;
  long long metricTimeElapsed = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
  if (metricTimeElapsed < _cloudConfig.metricsCollectionInterval) return;

  auto metrics = NativeInterface::get_metrics();
  if (metrics == nullptr) {
    LOG_TO_DEBUG("Metrics: {} ::: Device: %s", _config.deviceId.c_str());
    return;
  }
  _metricsAgent.LOGMETRICS(DYNAMICDEVICEMETRICS, metrics);
  _lastMetricTime = std::chrono::high_resolution_clock::now();
  free(metrics);
}

void CommandCenter::send_metrics(bool initComplete) {
  _metricsAgent.send_pending_logs(initComplete);
}

void CommandCenter::set_cloud_config() {
  if (_cloudConfigSet) return;
  if (_cloudConfigFetchRetries == 0) return;
  _cloudConfigFetchRetries--;

  auto cloudConfigResponse = ServerAPI::get_cloud_config();

  nlohmann::json pingMetric;
  pingMetric["pingTime"] = cloudConfigResponse.timeInMicros;
  log_metrics(PINGMETRIC, pingMetric);

  if (cloudConfigResponse.base.status != SUCCESS) return;

  _cloudConfig.logKey = cloudConfigResponse.logKey;
  _cloudConfig.metricsKey = cloudConfigResponse.metricsKey;
  _cloudConfig.logUrl = cloudConfigResponse.logUrl;
  _cloudConfig.logTimeInterval = cloudConfigResponse.logTimeInterval;
  _cloudConfig.metricsTimeInterval = cloudConfigResponse.metricsTimeInterval;
  logger.update_log_config(_cloudConfig.logKey, _cloudConfig.logTimeInterval, _cloudConfig.logUrl);
  _metricsAgent.update_log_config(_cloudConfig.metricsKey, _cloudConfig.metricsTimeInterval,
                                  _cloudConfig.logUrl);
  _cloudConfigSet = true;
}

void CommandCenter::log_metrics(const char* metricType, const nlohmann::json& metric) {
  _metricsAgent.LOGMETRICS(metricType, metric.dump().c_str());
}

void CommandCenter::write_log(const std::string& log) {
  if (!_initializeSuccess) {
    return;
  }
  _metricsAgent.OUTSIDELOG("%s", log.c_str());
}

void CommandCenter::write_metric(const char* metricType, const char* metricJson) {
  if (!_initializeSuccess) {
    return;
  }
  _metricsAgent.LOGMETRICS(metricType, metricJson);
}

bool CommandCenter::add_user_event(const string& eventMapJsonString, const string& tableName) {
  if (!_initializeSuccess) {
    return false;
  }
  return get_userEventsManager()->add_event(eventMapJsonString, tableName);
}

bool CommandCenter::get_model_status(const std::string& modelId, ModelStatus* status) {
  if (!_initializeSuccess) {
    return false;
  }
  return _mlExecutor->get_model_status(modelId, status);
}

bool CommandCenter::get_inputs_for_inferId(const std::string& inferId, ONNXInferenceReturn* ret) {
  return _mlExecutor->get_inputs_for_inferId(inferId, ret);
}