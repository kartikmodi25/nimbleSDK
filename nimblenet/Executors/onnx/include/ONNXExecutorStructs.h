#include "UserEventsStruct.h"
#include "nlohmann_json.h"
using json = nlohmann::json;
#include <vector>

struct TensorInfo {
  std::string name;
  int dataType;
  std::string featureName;
  bool isFeature = false;
  std::vector<int64_t> shape;
  int size;
  json preprocessorJson;
  bool toPreprocess = false;
};

struct PreProcessorInputInfo {
  std::string name;
  std::vector<std::string> inputNames;
};

struct ONNXModelInfo {
  bool valid = false;
  std::vector<TensorInfo> inputs;
  std::vector<TensorInfo> outputs;
  std::vector<PreProcessorInputInfo> preprocessorInputs;
};

struct ONNXPreProcessorInput {
  std::shared_ptr<ModelInput> modelInput;
  TensorInfo* tensorInfoPtr;

  ONNXPreProcessorInput(std::shared_ptr<ModelInput> modelInput_, TensorInfo* tensorInfo_) {
    tensorInfoPtr = tensorInfo_;
    modelInput = modelInput_;
  }
};

void from_json(const json& j, TensorInfo& tensorInfo);
void from_json(const json& j, PreProcessorInputInfo& preProcessorInputInfo);
void from_json(const json& j, ONNXModelInfo& modelInfo);