#include "ServerAPIStructs.h"

void from_json(const json& j, ErrorDetails& error) {
  j.at("description").get_to(error.description);
}

void from_json(const json& j, BaseAPIResponse& base) {
  j.at("status").get_to(base.status);
  if (base.status != SUCCESS) {
    j.at("error").get_to(base.error);
  }
}

void from_json(const json& j, RegisterResponse& registerResponse) {
  from_json(j, registerResponse.base);
  if (registerResponse.base.status == SUCCESS) {
    j.at("token").get_to(registerResponse.token);
  }
}

void from_json(const json& j, GetModelResponse& getModelResponse) {
  from_json(j, getModelResponse.base);
  if (getModelResponse.base.status == SUCCESS) {
    j.at("modelWeights").get_to(getModelResponse.modelWeights);
    j.at("modelVersion").get_to(getModelResponse.version);
  }
}

void from_json(const json& j, VersionCheckResponse& versionCheckResponse) {
  from_json(j, versionCheckResponse.base);
  if (versionCheckResponse.base.status == SUCCESS) {
    j.at("fileParts").get_to(versionCheckResponse.fileParts);
    j.at("version").get_to(versionCheckResponse.version);
    j.at("features").get_to(versionCheckResponse.extras);
  }
}

void from_json(const json& j, FeatureVersionCheckResponse& featureVersionCheckResponse) {
  from_json(j, featureVersionCheckResponse.base);
  if (featureVersionCheckResponse.base.status == SUCCESS) {
    j.at("version").get_to(featureVersionCheckResponse.version);
    j.at("dataType").get_to(featureVersionCheckResponse.dataType);
    j.at("expiry").get_to(featureVersionCheckResponse.expiry);
  }
}

void from_json(const json& j, CloudConfigResponse& cloudConfigResponse) {
  from_json(j, cloudConfigResponse.base);
  if (cloudConfigResponse.base.status == SUCCESS) {
    if (j.find("logKey") != j.end()) {
      j.at("logKey").get_to(cloudConfigResponse.logKey);
    }
    if (j.find("metricsKey") != j.end()) {
      j.at("metricsKey").get_to(cloudConfigResponse.metricsKey);
    }
    if (j.find("logUrl") != j.end()) {
      j.at("logUrl").get_to(cloudConfigResponse.logUrl);
    }
    if (j.find("metricsInterval") != j.end()) {
      j.at("metricsInterval").get_to(cloudConfigResponse.metricsTimeInterval);
    }
    if (j.find("logInterval") != j.end()) {
      j.at("logInterval").get_to(cloudConfigResponse.logTimeInterval);
    }
    if (j.find("metricsCollectionTimeInterval") != j.end()) {
      j.at("metricsCollectionTimeInterval").get_to(cloudConfigResponse.metricsCollectionInterval);
    }
  }
}

// Auth Info
void from_json(const json& j, AuthenticationInfo& authInfo) {
  j.at("apiHeader").get_to(authInfo.apiHeader);
  authInfo.valid = true;
}

void to_json(json& j, const AuthenticationInfo& authInfo) {
  j = json{{"apiHeader", authInfo.apiHeader}};
}