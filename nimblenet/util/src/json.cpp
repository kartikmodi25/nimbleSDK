/* This script is for load the JSON string and parse all the attributes. */
#include "nlohmann_json.h"
#include <string>

#include "util.h"
using json = nlohmann::json;

namespace JSONParser {
bool get_json(json& j, const std::string& s) {
  try {
    j = json::parse(s);
    return true;
  } catch (json::exception& e) {
    LOG_TO_FATAL("String is not a valid json %s. error=%s", s.c_str(), e.what());
  }
  return false;
}
}  // namespace JSONParser