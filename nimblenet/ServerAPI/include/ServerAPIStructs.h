#pragma once
#include <string>

#include "nlohmann_json.h"
#include "util.h"

using json = nlohmann::json;

struct BaseAPIRequest {
  std::string deviceId;
  std::string clientId;

  BaseAPIRequest(const std::string& device, const std::string& client) {
    deviceId = device;
    clientId = client;
  }
};

struct RegisterRequest {
  BaseAPIRequest base;
  std::vector<std::string> modelIds;

  RegisterRequest(const BaseAPIRequest& base_, const std::vector<std::string>& models_)
      : base(base_) {
    modelIds = models_;
  }
};

struct ModelRequest {
  BaseAPIRequest base;
  int partNum;
  std::string version;

  ModelRequest(const BaseAPIRequest& base_, int pNum, const std::string& v) : base(base_) {
    partNum = pNum;
    version = v;
  }
};

struct PlanRequest {
  BaseAPIRequest base;
  int partNum;
  std::string version;
  std::string modelId;

  PlanRequest(const BaseAPIRequest& base_, int pNum, const std::string& modelId_,
              const std::string& v)
      : base(base_) {
    modelId = modelId_;
    partNum = pNum;
    version = v;
  }
};

struct VersionRequest {
  BaseAPIRequest base;
  std::string dataType;
  std::string modelId;

  VersionRequest(const BaseAPIRequest& base_, std::string dType, const std::string& modelId_)
      : base(base_) {
    dataType = dType;
    modelId = modelId_;
  }
};

struct ErrorDetails {
  std::string description;
};

struct BaseAPIResponse {
  int status = JSON_PARSE_ERR;
  ErrorDetails error;
};

struct RegisterResponse {
  BaseAPIResponse base;
  std::string token;
};

struct GetModelResponse {
  BaseAPIResponse base;
  std::string modelWeights;
  std::string version;
};

struct VersionCheckResponse {
  BaseAPIResponse base;
  int fileParts;
  std::string version;
  std::string extras;
};

struct CloudConfigResponse {
  BaseAPIResponse base;
  std::string logKey;
  std::string logUrl;
  int logTimeInterval;
  std::string metricsKey;
  int metricsTimeInterval;
  int metricsCollectionInterval;
  /* added TimeInMicros here, as ServerAPI does not have commandCenter->writeMetric,
    and so this value needs to be passed to CommandCenter without the time of json parsing
  */
  int timeInMicros;

  CloudConfigResponse() {
    logKey = "";
    metricsKey = "";
    logUrl = DEFAULT_UPLOAD_LOGS_URL;
    logTimeInterval = LOG_TIMER_INTERVAL_IN_SECONDS;
    metricsTimeInterval = METRICS_TIMER_INTERVAL_IN_SECONDS;
    metricsCollectionInterval = METRICS_COLLECTION_TIME_INTERVAL_IN_SECONDS;
  }
};

struct FeatureVersionCheckResponse {
  BaseAPIResponse base;
  std::string version;
  int dataType;
  uint64_t expiry;
};

struct LogRequestBody {
  std::string host;
  json headers;
  json body;

  LogRequestBody(const json& logheaders, const json& logbody, const std::string& hostendpoint) {
    host = hostendpoint;
    body = logbody;
    headers = logheaders;
  }
};

struct AuthenticationInfo {
  bool valid = false;
  std::string apiHeader;
};

void from_json(const json& j, CloudConfigResponse& logKeyResponse);
void from_json(const json& j, RegisterResponse& registerResponse);
void from_json(const json& j, GetModelResponse& getModelResponse);
void from_json(const json& j, VersionCheckResponse& versionCheckResponse);
void from_json(const json& j, FeatureVersionCheckResponse& featureVersionCheckResponse);
void from_json(const json& j, BaseAPIResponse& base);
void from_json(const json& j, ErrorDetails& error);

// AuthenticationInfo
void from_json(const json& j, AuthenticationInfo& info);
void to_json(json& j, const AuthenticationInfo& authInfo);