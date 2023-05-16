#include "ResourceManagerStructs.h"
using json = nlohmann::json;

void to_json(json& j, const ModelData& modelData) {
  j = json{{"version", modelData.version}, {"model", modelData.model}};
}

void from_json(const json& j, ModelData& modelData) {
  j.at("model").get_to(modelData.model);
  j.at("version").get_to(modelData.version);
}

void to_json(json& j, const PlanData& planData) {
  j = json{{"version", planData.version},
           {"extras", planData.extraFields},
           {"planLength", planData.planLength}};
}

void from_json(const json& j, PlanData& planData) {
  j.at("version").get_to(planData.version);
  j.at("extras").get_to(planData.extraFields);
  j.at("planLength").get_to(planData.planLength);
}

void to_json(json& j, const FeatureData& featureData) {
  j = json{{"version", featureData.version},
           {"dataType", featureData.dataType},
           {"featureLength", featureData.featureLength},
           {"expiry", featureData.expiry}};
}

void from_json(const nlohmann::json& j, FeatureData& featureData) {
  j.at("version").get_to(featureData.version);
  j.at("dataType").get_to(featureData.dataType);
  j.at("featureLength").get_to(featureData.featureLength);
  j.at("expiry").get_to(featureData.expiry);
  featureData.valid = true;
}
