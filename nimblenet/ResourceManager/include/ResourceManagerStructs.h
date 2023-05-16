#pragma once
#include <nlohmann/json.hpp>

struct ModelData {
  bool valid = false;
  std::string version;
  std::string model;
};

struct PlanData {
  bool valid = false;
  std::string version;
  std::string plan;
  std::string extraFields;
  int planLength;
};

struct FeatureData {
  bool valid = false;
  char* feature = nullptr;
  int featureLength = 0;
  std::string version;
  int dataType = -1;
  uint64_t expiry = 0;
};

void from_json(const nlohmann::json& j, ModelData& modelData);
void to_json(nlohmann::json& j, const ModelData& modelData);
void from_json(const nlohmann::json& j, PlanData& planData);
void to_json(nlohmann::json& j, const PlanData& planData);
void from_json(const nlohmann::json& j, FeatureData& featureData);
void to_json(nlohmann::json& j, const FeatureData& featureData);