/* this script is loading the user config and
JSON parser to get the config attributes */

#include "ConfigManager.h"

#include "nlohmann_json.h"
#include "util.h"

using namespace std;
using json = nlohmann::json;

Config ConfigManager::load_user_config(const std::string& configJson) {
  /* Config Manager that load user info from JSON object */

  Config config = JSONParser::get<Config>(configJson);
  LOG_TO_INFO("Initialize config={%s}", config.c_str());
  return config;
}

void from_json(const json& j, Config& config) {
  /* Config JSON Attributes parse */
  j.at("host").get_to(config.host);
  j.at("deviceID").get_to(config.deviceId);
  j.at("clientID").get_to(config.clientId);
  j.at("modelIDs").get_to(config.modelIds);
  j.at("clientSecret").get_to(config.clientSecret);

  if (j.find("featureStores") != j.end()) {
    j.at("featureStores").get_to(config.featureStores);
  }

  if (j.find("databaseConfig") != j.end()) {
    j.at("databaseConfig").get_to(config.tableInfos);
  }

  if (j.find("maxInputsToSave") != j.end()) {
    j.at("maxInputsToSave").get_to(config.maxInputsToSave);
  }

  while (config.host.back() == '/') {
    config.host = config.host.substr(0, config.host.size() - 1);
  }
  if (j.find("debug") != j.end()) {
    j.at("debug").get_to(config.debug);
  }
  config.valid = true;
}