#pragma once
#include <nlohmann/json.hpp>
#include <string>

#include "CommandCenter.h"
#include "ConfigManager.h"
#include "ResourceManagerStructs.h"

class CommandCenter;

class ResourceManager {
  CommandCenter* _commandCenter;
  PlanData get_inference_plan_data_from_device(const std::string& modelId);
  ModelData get_model_data_from_device(const std::string& modelId);
  FeatureData get_feature_data_from_device(const std::string& featureName);
  bool save_inference_plan_data_on_device(const std::string& modelId, const PlanData& planData);
  bool save_model_data_on_device(const std::string& modelId, const ModelData& modelData);
  bool save_feature_data_on_device(const std::string& featureName, const FeatureData& featureData);
  int load_plan(const std::string& modelId);
  bool load_model(const std::string& modelId);
  bool load_feature(const std::string& featureName);

 public:
  bool load_plan_from_device(const std::string& modelId);
  bool load_feature_from_device(const std::string& featureName);
  void update_plan_if_required(const std::string& modelId);
  void update_feature_if_required(const std::string& featureName);
  bool load_model_and_plan(const std::string& modelId);
  bool load_feature_store(const std::string& featureName);
  bool check_model_loaded_for_inference(const std::string& modelId);
  ResourceManager(CommandCenter* commandCenter);
};