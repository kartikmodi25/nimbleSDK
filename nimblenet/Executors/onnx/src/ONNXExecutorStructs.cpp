#include "ONNXExecutorStructs.h"
using json = nlohmann::json;

void from_json(const json& j, TensorInfo& tensorInfo) {
  j.at("name").get_to(tensorInfo.name);
  j.at("dataType").get_to(tensorInfo.dataType);
  if (j.find("featureStore") == j.end()) {
    tensorInfo.isFeature = false;
  } else {
    tensorInfo.isFeature = true;
    j.at("featureStore").get_to(tensorInfo.featureName);
  }
  if (j.find("processor") == j.end()) {
    tensorInfo.toPreprocess = false;
  } else {
    tensorInfo.toPreprocess = true;
    j.at("processor").get_to(tensorInfo.preprocessorJson);
  }
  j.at("shape").get_to(tensorInfo.shape);
  tensorInfo.size = 1;
  for (const auto val : tensorInfo.shape) {
    tensorInfo.size *= val;
  }
}

void from_json(const json& j, ONNXModelInfo& modelInfo) {
  j.at("inputs").get_to(modelInfo.inputs);
  j.at("outputs").get_to(modelInfo.outputs);
  if (j.find("inputsToProcessors") != j.end())
    j.at("inputsToProcessors").get_to(modelInfo.preprocessorInputs);
  modelInfo.valid = true;
}

void from_json(const json& j, PreProcessorInputInfo& preProcessorInputInfo) {
  j.at("name").get_to(preProcessorInputInfo.name);
  j.at("inputNames").get_to(preProcessorInputInfo.inputNames);
}