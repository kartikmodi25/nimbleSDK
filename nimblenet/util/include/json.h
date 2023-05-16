#pragma once
#include <nlohmann/json.hpp>
#include <string>

#include "Logger.h"
// #include <iostream>
using json = nlohmann::json;

namespace JSONParser {
bool get_json(json& j, const std::string& s);

template <class T>
T get_from_json(const json& j) {
  try {
    return j.get<T>();
  } catch (json::exception& e) {
    LOG_TO_FATAL("JSON object=%s could not converted to object of type=%s. error=%s",
                 j.dump().c_str(), typeid(T).name(), e.what());
  }
  return T();
}

template <class T>
T get(const std::string& jsonString) {
  json j;
  if (!get_json(j, jsonString)) {
    return T();
  }
  return get_from_json<T>(j);
}

}  // namespace JSONParser
