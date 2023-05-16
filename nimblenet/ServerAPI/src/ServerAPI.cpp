/* This script handle the server or network call for load,
download, and register device and versioning. */

#include "ServerAPI.h"

#include <nlohmann/json.hpp>

#include "ConfigManager.h"
#include "util.h"

using json = nlohmann::json;
#include "NativeInterface.h"

using namespace std;

string ServerAPI::HEADER = "";
string ServerAPI::HOST = "";
Config ServerAPI::_config;
int ServerAPI::registerRetries = DEFAULT_REGISTER_RETRIES;
bool ServerAPI::registerDone = false;

bool ServerAPI::init(const Config& config) {
  if (registerDone) return true;
  if (registerRetries == 0) return false;
  registerRetries--;
  HOST = config.host;
  _config = config;
  string authInfoString;
  if (NativeInterface::get_file_from_device("AuthInfo.txt", authInfoString)) {
    AuthenticationInfo info = JSONParser::get<AuthenticationInfo>(authInfoString);
    if (info.valid) {
      HEADER = info.apiHeader;
      return registerDone = true;
    }
  }
  if (device_register()) {
    return registerDone = true;
  }
  LOG_TO_ERROR("%s", "Registeration failed");
  return false;
}

void ServerAPI::reset_register_retries() { registerRetries = DEFAULT_REGISTER_RETRIES; }

GetModelResponse ServerAPI::get_model(const ModelRequest& modReq, int retries) {
  string apiPoint = "v1/getmodel";
  apiPoint += "/clientId/" + modReq.base.clientId + "/deviceId/" + modReq.base.deviceId +
              "/numPart/" + to_string(modReq.partNum) + "/version/" + modReq.version;
  string URL = HOST + apiPoint;
  string responseString = (*NativeInterface::send_request("", HEADER, URL, "GET")).r.body;
  GetModelResponse r = JSONParser::get<GetModelResponse>(responseString);

  if (r.base.status == AUTH_ERR) {
    if (retries > 0) {
      LOG_TO_DEBUG("%s", "Retrying get_model api call because of AuthError");
      if (device_register()) return get_model(modReq, retries - 1);
    }
  }
  return r;
}

const std::shared_ptr<NetworkResponse> ServerAPI::get_plan(const PlanRequest& planReq,
                                                           int retries) {
  string URL = HOST + HTTPSERVICE + API_VERSION + "/clients/" + planReq.base.clientId + "/models/" +
               planReq.modelId + "/versions/" + planReq.version + "/parts/" +
               to_string(planReq.partNum) + "?planType=inference";
  const auto response = NativeInterface::send_request("", HEADER, URL, "GET");

  if (response->r.statusCode == AUTH_ERR) {
    if (retries > 0) {
      LOG_TO_DEBUG("%s", "Retrying get_plan api call because of AuthError");
      if (device_register()) return get_plan(planReq, retries - 1);
    }
  }
  return response;
}

bool ServerAPI::device_register() {
  json body;
  body["deviceId"] = _config.deviceId;
  body["clientId"] = _config.clientId;
  body["modelNames"] = _config.modelIds;
  json registerHeader;
  registerHeader["ClientSecret"] = _config.clientSecret;
  string URL = HOST + HTTPSERVICE + API_VERSION + "/clients/" + _config.clientId + "/register";
  const auto netResponse =
      NativeInterface::send_request(body.dump(), registerHeader.dump(), URL, "POST");
  string responseString(netResponse->r.body, netResponse->r.bodyLength);
  RegisterResponse response = JSONParser::get<RegisterResponse>(responseString);

  if (response.base.status != SUCCESS) {
    LOG_TO_ERROR("Device Registration Failed with status_code=%d, error=%s", response.base.status,
                 response.base.error.description.c_str());
    return false;
  }

  string JWT_TOKEN = response.token;
  json header;
  header["AuthorizationId"] = string("Bearer ") + JWT_TOKEN;
  HEADER = header.dump();
  AuthenticationInfo info;
  info.apiHeader = HEADER;
  NativeInterface::save_file_on_device(nlohmann::json(info).dump(), "AuthInfo.txt");
  LOG_TO_INFO("%s", "Device Registration Successful");

  return true;
}

VersionCheckResponse ServerAPI::version_check(const VersionRequest& verReq, int retries) {
  string URL = HOST + HTTPSERVICE + API_VERSION + "/clients/" + verReq.base.clientId + "/models/" +
               verReq.modelId + "/version";

  const auto response = NativeInterface::send_request("", HEADER, URL, "GET");

  string responseString(response->r.body, response->r.bodyLength);
  if (response->r.statusCode == AUTH_ERR) {
    if (retries > 0) {
      LOG_TO_DEBUG("%s", "Retrying version_check api call because of AuthError");
      if (device_register()) return version_check(verReq, retries - 1);
    }
  }
  VersionCheckResponse r = JSONParser::get<VersionCheckResponse>(responseString);
  return r;
}

bool ServerAPI::upload_logs(const LogRequestBody& logrequest) {
  string host = logrequest.host;
  json body = logrequest.body;
  json headers = logrequest.headers;
  const auto response = NativeInterface::send_request(body.dump(), headers.dump(), host, "POST");
  string responseString(response->r.body, response->r.bodyLength);
  if (response->r.statusCode != 202) {  // 202 is datadog success code
    LOG_TO_DEBUG("Log File Upload Unsuccessful %s, statusCode=%d", responseString.c_str(),
                 response->r.statusCode);
    return false;
  }
  LOG_TO_DEBUG("Log File Upload success %s", responseString.c_str());
  return true;
}

CloudConfigResponse ServerAPI::get_cloud_config(int retries) {
  string URL = HOST + HTTPSERVICE + API_VERSION + "/clients/" + _config.clientId + "/events/config";
  auto start = std::chrono::high_resolution_clock::now();
  const auto response = NativeInterface::send_request("", HEADER, URL, "GET");
  auto stop = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
  string responseString(response->r.body, response->r.bodyLength);
  CloudConfigResponse configResponse;
  if (response->r.statusCode == AUTH_ERR) {
    if (retries > 0) {
      LOG_TO_DEBUG("%s", "Retrying get_cloud_config api call because of AuthError");
      if (device_register()) return get_cloud_config(retries - 1);
    }
  }
  configResponse = JSONParser::get<CloudConfigResponse>(responseString);
  configResponse.timeInMicros = duration.count();
  if (configResponse.base.status == SUCCESS) {
    LOG_TO_DEBUG("%s", "Found Cloud Config");
    configResponse.metricsKey = configResponse.logKey;
    return configResponse;
  }
  LOG_TO_ERROR("Could not get Cloud Config for client:%s from Cloud, statusCode=%d, error=%s",
               _config.clientId.c_str(), configResponse.base.status,
               configResponse.base.error.description.c_str());
  return configResponse;
}

const std::shared_ptr<NetworkResponse> ServerAPI::get_feature(const std::string& featureName,
                                                              const std::string featureVersion,
                                                              int retries) {
  string URL = HOST + FEATURESERVICE + API_VERSION + "/clients/" + _config.clientId + "/features/" +
               featureName + "/versions/" + featureVersion;
  const auto response = NativeInterface::send_request("", HEADER, URL, "GET");
  if (response->r.statusCode == AUTH_ERR) {
    if (retries > 0) {
      LOG_TO_DEBUG("%s", "Retrying get_feature api call because of AuthError");
      if (device_register()) return get_feature(featureName, featureVersion, retries - 1);
    }
  }
  return response;
}

FeatureVersionCheckResponse ServerAPI::get_feature_version(const std::string& featureName,
                                                           int retries) {
  string URL = HOST + FEATURESERVICE + API_VERSION + "/clients/" + _config.clientId + "/features/" +
               featureName + "/version";
  const auto response = NativeInterface::send_request("", HEADER, URL, "GET");
  string responseString(response->r.body, response->r.bodyLength);
  if (response->r.statusCode == AUTH_ERR) {
    if (retries > 0) {
      LOG_TO_DEBUG("%s", "Retrying get_feature_version api call because of AuthError");
      if (device_register()) return get_feature_version(featureName, retries - 1);
    }
  }
  FeatureVersionCheckResponse r = JSONParser::get<FeatureVersionCheckResponse>(responseString);
  return r;
}