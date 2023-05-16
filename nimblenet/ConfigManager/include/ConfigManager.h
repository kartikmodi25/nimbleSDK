#include <string>
#include <vector>

#include "nlohmann_json.h"

using json = nlohmann::json;
#pragma once

struct Config {
  bool valid = false;
  std::string deviceId;
  std::string clientId;
  std::string host;
  std::string clientSecret;
  std::vector<std::string> modelIds;
  std::vector<std::string> featureStores;
  std::vector<json> tableInfos;
  bool debug = false;
  int maxInputsToSave = 0;

  char* c_str() {
    std::string tables = "[";
    for (const auto& it : tableInfos) {
      tables += it.dump() + ",";
    }
    tables += "]";
    std::string models = "[";
    for (const auto& model : modelIds) {
      models += model + ",";
    }
    models += "]";
    std::string features = "[";
    for (const auto& feature : featureStores) {
      features += feature + ",";
    }
    features += "]";
    char* ret;
    asprintf(&ret,
             "valid=%s,deviceId=%s,clientId=%s,clientSecret=****,host=%s,modelIds=%s,featureStores="
             "%s, databaseConfig=%s, debug:%s, maxInputsToSave:%d",
             valid ? "true" : "false", deviceId.c_str(), clientId.c_str(), host.c_str(),
             models.c_str(), features.c_str(), tables.c_str(), debug ? "true" : "false",
             maxInputsToSave);
    return ret;
  }

  bool isDebug() const { return debug; }
};

void from_json(const json& j, Config& config);

class ConfigManager {
 public:
  static Config load_user_config(const std::string& configJson);
};
