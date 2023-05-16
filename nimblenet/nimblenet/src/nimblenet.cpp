#include "nimblenet.h"

#include "CommandCenter.h"
#include "ConfigManager.h"
#include "NativeInterface.h"
#include "util.h"

using namespace std;

CommandCenter commandCenter;
Logger logger;

int initialize_nimblenet(const char* configJson, const char* homeDirectory) {
  bool initLogger = logger.init_logger(string(homeDirectory) + "/logs");

  // Do not initialize nimbleSDK if the logger is unable to initialize
  if (!initLogger) return 0;

  NativeInterface::HOMEDIR = homeDirectory;
  Config config = ConfigManager::load_user_config(configJson);
  if (!config.valid) {
    LOG_TO_ERROR("Error parsing the config %s", configJson);
    return 0;
  }
  logger.update_device_details(config.deviceId, config.clientId, config.debug);
  return commandCenter.initialize(config);
}

int get_inference(const char* modelId, const InferenceRequest req, InferenceReturn* ret,
                  const char* inferId) {
  return commandCenter.get_inference(modelId, req, ret, inferId);
}

void exit_inference(const char* modelId, const char* inferId, bool success) {
  commandCenter.exit_inference(modelId, inferId, success);
}

void write_log(const char* log) { commandCenter.write_log(log); }

void write_metric(const char* metricType, const char* metricJson) {
  commandCenter.write_metric(metricType, metricJson);
}

bool add_event(const char* eventMapJsonString, const char* tableName) {
  return commandCenter.add_user_event(eventMapJsonString, tableName);
}

bool get_model_status(const char* modelId, ModelStatus* status) {
  return commandCenter.get_model_status(modelId, status);
}

void internet_switched_on() { return commandCenter.internet_switched_on(); };

bool get_inputs_for_inferId(const char* inferId, ONNXInferenceReturn* ret) {
  return commandCenter.get_inputs_for_inferId(inferId, ret);
}