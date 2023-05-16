#pragma once
#include <string.h>

#include "ConfigManager.h"
#include "NativeInterfaceStructs.h"
#include "ServerAPIStructs.h"
#include "util.h"

class ServerAPI {
  static std::string HEADER;
  static std::string HOST;
  static Config _config;
  static int registerRetries;
  static bool registerDone;

 public:
  ServerAPI() {}

  ~ServerAPI() {}

  static GetModelResponse get_model(const ModelRequest& modReq,
                                    int retries = DEFAULT_AUTH_ERROR_RETRIES);
  static const std::shared_ptr<NetworkResponse> get_plan(const PlanRequest& planReq,
                                                         int retries = DEFAULT_AUTH_ERROR_RETRIES);
  static bool device_register();
  static VersionCheckResponse version_check(const VersionRequest& verReq,
                                            int retries = DEFAULT_AUTH_ERROR_RETRIES);
  static bool upload_logs(const LogRequestBody& logrequest);
  static bool init(const Config& config);
  static CloudConfigResponse get_cloud_config(int retries = DEFAULT_AUTH_ERROR_RETRIES);
  static const std::shared_ptr<NetworkResponse> get_feature(
      const std::string& featureName, const std::string featureVersion,
      int retries = DEFAULT_AUTH_ERROR_RETRIES);
  static FeatureVersionCheckResponse get_feature_version(const std::string& featureName,
                                                         int retries = DEFAULT_AUTH_ERROR_RETRIES);
  static void reset_register_retries();
};
