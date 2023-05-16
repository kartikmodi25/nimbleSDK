#pragma once
#include <map>
#include <mutex>
#include <string>

#include "CommandCenter.h"
#include "ExecutorStructs.h"
#include "ONNXExecutorStructs.h"
#include "ResourceManagerStructs.h"
#include "onnx.h"
#include "util.h"

class CommandCenter;

class ONNXModel {
  CommandCenter* _commandCenter;
  Ort::Session* _session = nullptr;
  ONNXModelInfo _info;
  std::string _modelId;
  Ort::SessionOptions _sessionOptions;
  std::mutex _modelMutex;
  std::vector<Ort::Value> _outputTensors;
  std::vector<Ort::Value> _inputTensors;
  std::string _version;
  Ort::MemoryInfo _memoryInfo;
  std::map<std::string, int> _inputNamesToIdMap;
  bool check_input(const std::string& inferId, int inputIndex, int dataType, int inputSizeBytes);
  Ort::Value create_ort_tensor(int inputIndex, void* inputData);
  void run_dummy_inference();
  bool allocate_output_memory(ONNXInferenceReturn* ret);

 public:
  static int get_field_size_from_data_type(int dataType);
  ONNXModel(const ONNXModelInfo& modelInfo, const std::string& plan, Ort::Env& mEnv,
            const std::string& version, const std::string& modelId, CommandCenter* commandCenter);
  ~ONNXModel();

  int get_inference(const std::string& inferId, const ONNXInferenceRequest& req,
                    ONNXInferenceReturn* ret,
                    std::vector<ONNXPreProcessorInput>& preprocessorInputsToFill);

  const std::string get_plan_version() { return _version; }

  void get_model_status(ModelStatus* status);
};

class InferenceInputsDatabase {
  std::vector<std::vector<ONNXPreProcessorInput>> _preprocessorInputsForInferIds;
  std::vector<std::string> _inferIds;
  int _overWriteInferenceIndex;
  int _maxInferencesSaved;

  bool populate_preprocessors_input_in_return_object(
      const std::vector<ONNXPreProcessorInput>& preprocessorInputs, ONNXInferenceReturn* ret);

 public:
  InferenceInputsDatabase(int maxInferencesSaved) {
    _maxInferencesSaved = maxInferencesSaved;
    _preprocessorInputsForInferIds =
        std::vector<std::vector<ONNXPreProcessorInput>>(maxInferencesSaved);
    _inferIds = std::vector<std::string>(maxInferencesSaved);
    _overWriteInferenceIndex = 0;
  }

  void add_input_for_inferId(const std::string& inferId,
                             const std::vector<ONNXPreProcessorInput>& preprocessorInputs) {
    if (_maxInferencesSaved == 0) return;
    _preprocessorInputsForInferIds[_overWriteInferenceIndex] = std::move(preprocessorInputs);
    _inferIds[_overWriteInferenceIndex] = inferId;
    _overWriteInferenceIndex = (_overWriteInferenceIndex + 1) % _maxInferencesSaved;
  }

  bool get_inputs_for_inferId(const std::string& inferId, ONNXInferenceReturn* ret);
};

class ONNXExecutor {
  CommandCenter* _commandCenter;
  Ort::Env _onxEnv;
  std::map<std::string, std::shared_ptr<ONNXModel>> modelMap;
  std::mutex _modelMapMutex;
  std::map<std::string, int> _loadModelRetries;
  std::map<std::string, bool> _isModelUpdated;
  InferenceInputsDatabase _inferenceDataBase;

 public:
  ONNXExecutor(CommandCenter* commandCenter);

  bool is_model_required() { return false; }

  bool check_plan_loaded(const std::string& modelId);

  bool check_model_loaded(const std::string& modelId) const { return true; }

  bool load_model(const std::string& model, const std::string& modelId) const { return true; }

  bool load_plan(const std::string& modelId, const PlanData& planData);
  int get_inference(const std::string& inferId, const std::string& modelId,
                    const ONNXInferenceRequest& req, ONNXInferenceReturn* ret);
  const std::string get_plan_version(const std::string& modelId);
  void update_model_retries(const std::string& modelId);
  void reset_model_retries(const std::string& modelId);
  bool can_model_retry(const std::string& modelId);
  void plan_updated(const std::string& modelId, bool isUpdated);
  bool get_model_status(const std::string& modelId, ModelStatus* status);
  bool get_inputs_for_inferId(const std::string& inferId, ONNXInferenceReturn* ret);
};
