#include "Executor.h"
#include "nlohmann_json.h"
#include "util.h"

using json = nlohmann::json;

ONNXExecutor::ONNXExecutor(CommandCenter* commandCenter)
    : _inferenceDataBase(commandCenter->get_config().maxInputsToSave) {
  _onxEnv = Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_VERBOSE, "ONNX Environment");
  _commandCenter = commandCenter;
  for (auto model : _commandCenter->get_config().modelIds) {
    _loadModelRetries[model] = DEFAULT_LOAD_MODEL_RETRIES;
    _isModelUpdated[model] = false;
  }
}

const std::string ONNXExecutor::get_plan_version(const std::string& modelId) {
  if (check_plan_loaded(modelId)) {
    return modelMap[modelId]->get_plan_version();
  } else
    return "";
}

void ONNXExecutor::reset_model_retries(const std::string& modelId) {
  if (_loadModelRetries.find(modelId) != _loadModelRetries.end()) {
    _loadModelRetries[modelId] = DEFAULT_LOAD_MODEL_RETRIES;
  }
}

void ONNXExecutor::update_model_retries(const std::string& modelId) {
  if (_loadModelRetries.find(modelId) != _loadModelRetries.end()) {
    _loadModelRetries[modelId]--;
    if (_loadModelRetries[modelId] < 0) {
      LOG_TO_DEBUG("No retries for model=%s left.", modelId.c_str());
    }
  }
}

bool ONNXExecutor::can_model_retry(const std::string& modelId) {
  if (check_plan_loaded(modelId) && _isModelUpdated[modelId]) {
    return false;
  }

  if (_loadModelRetries.find(modelId) != _loadModelRetries.end()) {
    return (_loadModelRetries[modelId] > 0);
  }
  return false;
}

bool ONNXExecutor::load_plan(const std::string& modelId, const PlanData& planData) {
  ONNXModelInfo info = JSONParser::get<ONNXModelInfo>(planData.extraFields);
  if (!info.valid) {
    LOG_TO_FATAL("ONNXModel info could not be parsed: %s", planData.extraFields.c_str());
    return false;
  }

  try {
    std::shared_ptr<ONNXModel> newModel = std::make_shared<ONNXModel>(
        info, planData.plan, _onxEnv, planData.version, modelId, _commandCenter);
    std::lock_guard<std::mutex> locker(_modelMapMutex);
    modelMap[modelId] = newModel;
  } catch (Ort::Exception e) {
    LOG_TO_ERROR("Exception in load_plan:%s with errorCode:%d, for modelId=%s, version=%s",
                 e.what(), e.GetOrtErrorCode(), modelId.c_str(), planData.version.c_str());
    return false;
  } catch (std::exception e) {
    LOG_TO_ERROR("Exception in creating ONNXModel for modelId=%s error=%s version=%s",
                 modelId.c_str(), e.what(), planData.version.c_str());
    return false;
  } catch (...) {
    LOG_TO_ERROR("Exception in creating ONNXModel for modelId=%s version=%s", modelId.c_str(),
                 planData.version.c_str());
    return false;
  }
  return true;
}

bool ONNXExecutor::check_plan_loaded(const std::string& modelId) {
  std::lock_guard<std::mutex> locker(_modelMapMutex);
  bool ret = modelMap.find(modelId) != modelMap.end();
  return ret;
}

void ONNXExecutor::plan_updated(const std::string& modelId, bool isUpdated) {
  if (_isModelUpdated.find(modelId) != _isModelUpdated.end()) {
    _isModelUpdated[modelId] = isUpdated;
  }
}

bool ONNXExecutor::get_inputs_for_inferId(const std::string& inferId, ONNXInferenceReturn* ret) {
  return _inferenceDataBase.get_inputs_for_inferId(inferId, ret);
}

bool InferenceInputsDatabase::populate_preprocessors_input_in_return_object(
    const std::vector<ONNXPreProcessorInput>& preprocessorInputs, ONNXInferenceReturn* ret) {
  ret->numOutputs = preprocessorInputs.size();
  ret->outputs = (void**)malloc(sizeof(void*) * ret->numOutputs);
  ret->outputNames = (char**)malloc(sizeof(char*) * ret->numOutputs);
  ret->outputTypes = (int*)malloc(sizeof(int) * ret->numOutputs);
  ret->outputShapes = (int**)malloc(sizeof(int*) * ret->numOutputs);
  ret->outputLengths = (int*)malloc(sizeof(int) * ret->numOutputs);
  ret->outputShapeLengths = (int*)malloc(sizeof(int) * ret->numOutputs);

  for (int i = 0; i < ret->numOutputs; i++) {
    ret->outputs[i] = nullptr;
    ret->outputShapes[i] = nullptr;
  }
  // iterate over outputs size to check the ONNX data type's to allocate memory for output
  for (int i = 0; i < ret->numOutputs; i++) {
    TensorInfo* t = preprocessorInputs[i].tensorInfoPtr;
    int fieldSize = ONNXModel::get_field_size_from_data_type(t->dataType);
    if (fieldSize == 0) return false;
    ret->outputs[i] = malloc(t->size * fieldSize);
    memcpy(ret->outputs[i], preprocessorInputs[i].modelInput->data, t->size * fieldSize);
    ret->outputShapeLengths[i] = t->shape.size();
    ret->outputShapes[i] = (int*)malloc(sizeof(int) * ret->outputShapeLengths[i]);
    for (int j = 0; j < t->shape.size(); j++) ret->outputShapes[i][j] = t->shape[j];
    ret->outputLengths[i] = t->size;
    ret->outputNames[i] = const_cast<char*>(t->name.c_str());
    ret->outputTypes[i] = t->dataType;
  }
  return true;
}

bool InferenceInputsDatabase::get_inputs_for_inferId(const std::string& inferId,
                                                     ONNXInferenceReturn* ret) {
  for (int i = 0; i < _maxInferencesSaved; i++) {
    if (_inferIds[i] == inferId) {
      return populate_preprocessors_input_in_return_object(_preprocessorInputsForInferIds[i], ret);
    }
  }
  return false;
}

int ONNXExecutor::get_inference(const std::string& inferId, const std::string& modelId,
                                const ONNXInferenceRequest& req, ONNXInferenceReturn* ret) {
  if (check_plan_loaded(modelId)) {
    std::vector<ONNXPreProcessorInput> preprocessorInputsToFill;
    auto inferStatus =
        modelMap[modelId]->get_inference(inferId, req, ret, preprocessorInputsToFill);
    _inferenceDataBase.add_input_for_inferId(inferId, std::move(preprocessorInputsToFill));
    return inferStatus;
  } else {
    LOG_TO_ERROR("Inference Id:%s, Model not found %s", inferId.c_str(), modelId.c_str());
    return TERMINAL_ERROR;
  }
}

bool ONNXExecutor::get_model_status(const std::string& modelId, ModelStatus* status) {
  if (check_plan_loaded(modelId)) {
    modelMap[modelId]->get_model_status(status);
    return true;
  }
  return false;
}

Ort::Value ONNXModel::create_ort_tensor(int inputIndex, void* inputData) {
  int fieldSize = get_field_size_from_data_type(_info.inputs[inputIndex].dataType);
  if (fieldSize == 0) return Ort::Value(nullptr);  // inputType not supported
  return Ort::Value::CreateTensor(_memoryInfo, inputData, fieldSize * _info.inputs[inputIndex].size,
                                  _info.inputs[inputIndex].shape.data(),
                                  _info.inputs[inputIndex].shape.size(),
                                  (ONNXTensorElementDataType)_info.inputs[inputIndex].dataType);
}

bool ONNXModel::check_input(const std::string& inferId, int inputIndex, int dataType,
                            int inputSizeBytes) {
  int fieldSize = get_field_size_from_data_type(dataType);
  if (fieldSize == 0) return false;  // inputType not supported
  int expectedSizeBytes = _info.inputs[inputIndex].size * fieldSize;
  if (expectedSizeBytes != inputSizeBytes) {
    LOG_TO_ERROR("Id:%s Inference: inputName=%s is of wrong length=%d, should be of length=%d",
                 inferId.c_str(), _info.inputs[inputIndex].name.c_str(), inputSizeBytes,
                 expectedSizeBytes);
    return false;
  } else if (_info.inputs[inputIndex].dataType != dataType) {
    LOG_TO_ERROR("Id:%s Inference: inputName=%s should be of dataType=%d, given input dataType=%d",
                 inferId.c_str(), _info.inputs[inputIndex].name.c_str(),
                 _info.inputs[inputIndex].dataType, dataType);
    return false;
  }
  return true;
}

int ONNXModel::get_field_size_from_data_type(int dataType) {
  switch (dataType) {
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT:
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32:
      return 4;
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE:
    case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64:
      return 8;
    default:
      LOG_TO_ERROR("get_field_size_from_data_type: No support for inputType=%d", dataType);
      return 0;
  }
}

// Memory allocated here must be freed by the consumer (android/ios). The consumer may not support
// new/delete, hence use malloc here.
bool ONNXModel::allocate_output_memory(ONNXInferenceReturn* ret) {
  ret->numOutputs = _info.outputs.size();
  ret->outputs = (void**)malloc(sizeof(void*) * ret->numOutputs);
  ret->outputNames = (char**)malloc(sizeof(char*) * ret->numOutputs);
  ret->outputTypes = (int*)malloc(sizeof(int) * ret->numOutputs);
  ret->outputShapes = (int**)malloc(sizeof(int*) * ret->numOutputs);
  ret->outputLengths = (int*)malloc(sizeof(int) * ret->numOutputs);
  ret->outputShapeLengths = (int*)malloc(sizeof(int) * ret->numOutputs);

  for (int i = 0; i < ret->numOutputs; i++) {
    ret->outputs[i] = nullptr;
    ret->outputShapes[i] = nullptr;
  }

  // iterate over outputs size to check the ONNX data type's to allocate memory for output
  for (int i = 0; i < ret->numOutputs; i++) {
    int fieldSize = get_field_size_from_data_type(_info.outputs[i].dataType);
    if (fieldSize == 0) return false;
    ret->outputs[i] = malloc(_info.outputs[i].size * fieldSize);
    _outputTensors.push_back(
        Ort::Value::CreateTensor(_memoryInfo, ret->outputs[i], fieldSize * _info.outputs[i].size,
                                 _info.outputs[i].shape.data(), _info.outputs[i].shape.size(),
                                 (ONNXTensorElementDataType)_info.outputs[i].dataType));
    ret->outputShapeLengths[i] = _info.outputs[i].shape.size();
    ret->outputShapes[i] = (int*)malloc(sizeof(int) * ret->outputShapeLengths[i]);
    for (int j = 0; j < _info.outputs[i].shape.size(); j++)
      ret->outputShapes[i][j] = _info.outputs[i].shape[j];
    ret->outputLengths[i] = _info.outputs[i].size;
    ret->outputNames[i] = const_cast<char*>(_info.outputs[i].name.c_str());
    ret->outputTypes[i] = _info.outputs[i].dataType;
  }
  return true;
}

template <class T>
std::string recursive_string(const std::vector<int64_t>& shape, int shapeDepth, const T* data,
                             int dataIndex, int totalSizeOfDepth) {
  if (shapeDepth == shape.size()) {
    return to_string(data[dataIndex]);
  }
  std::string output = "[";
  for (int i = 0; i < shape[shapeDepth]; i++) {
    output += recursive_string(shape, shapeDepth + 1, data,
                               dataIndex + i * totalSizeOfDepth / shape[shapeDepth],
                               totalSizeOfDepth / shape[shapeDepth]);
    if (i != shape[shapeDepth] - 1) {
      output += ",";
    }
  }
  output += "]";
  return output;
}

int ONNXModel::get_inference(const std::string& inferId, const ONNXInferenceRequest& req,
                             ONNXInferenceReturn* ret,
                             std::vector<ONNXPreProcessorInput>& preprocessorInputsToFill) {
  /* ARGS:
          ONNXInferenceRequest : req (request schema)
          ONNXInferenceReturn  : res (response schema)
      RETURNS: int
          inference performed for a set of user input config

  */
  _outputTensors.clear();
  std::vector<const char*> inputNames;
  std::vector<std::shared_lock<std::shared_mutex>> readLocks(_info.inputs.size());
  std::lock_guard<std::mutex> locker(_modelMutex);
  // creating inputTensors
  for (int i = 0; i < _info.inputs.size(); i++) {
    inputNames.push_back(_info.inputs[i].name.c_str());
    if (_info.inputs[i].toPreprocess) {
      // Filled later from input values from corresponding Preprocessor classes.
      continue;
    }
    if (_info.inputs[i].isFeature) {
      // loading inputs from featureStore
      auto featureName = _info.inputs[i].featureName;
      if (_commandCenter->get_feature_store_manager()->get_read_lock(featureName, readLocks[i])) {
        const auto feature = _commandCenter->get_feature_store_manager()->get_feature(featureName);
        if (!check_input(inferId, i, feature.dataType, feature.byteLength)) {
          return TERMINAL_ERROR;  // error logged in check_input
        }
        Ort::Value createdTensor = create_ort_tensor(i, (void*)feature.data);
        if (createdTensor.HasValue())
          _inputTensors[i] = std::move(createdTensor);
        else
          return TERMINAL_ERROR;  // error logged in create_ort_tensor
      } else {
        return RETRYABLE_ERROR;  // error logged in get_read_lock
      }
    } else {
      // loading client input
      int clientInputIndex = -1;
      for (int j = 0; j < req.numInputs; j++) {
        if (_info.inputs[i].name == req.inputs[j].name) {
          clientInputIndex = j;
          break;
        }
      }
      if (clientInputIndex == -1) {
        LOG_TO_ERROR("Id:%s Inference: inputName=%s not provided for model %s", inferId.c_str(),
                     _info.inputs[i].name.c_str(), _modelId.c_str());
        return TERMINAL_ERROR;
      }
      if (!check_input(inferId, i, req.inputs[clientInputIndex].dataType,
                       req.inputs[clientInputIndex].length *
                           get_field_size_from_data_type(req.inputs[clientInputIndex].dataType))) {
        return TERMINAL_ERROR;  // error logged in check_input
      }
      Ort::Value createdTensor = create_ort_tensor(i, req.inputs[clientInputIndex].data);
      if (createdTensor.HasValue())
        _inputTensors[i] = std::move(createdTensor);
      else
        return TERMINAL_ERROR;  // error already logged in create_ort_tensor
    }
  }
  auto start = std::chrono::high_resolution_clock::now();
  for (auto preprocessorInputInfo : _info.preprocessorInputs) {
    // loading client input
    int clientInputIndex = -1;
    for (int j = 0; j < req.numInputs; j++) {
      if (preprocessorInputInfo.name == req.inputs[j].name) {
        clientInputIndex = j;
        break;
      }
    }
    if (clientInputIndex == -1) {
      LOG_TO_ERROR("Id:%s Inference: preprocessorInputName=%s not provided for model %s",
                   inferId.c_str(), preprocessorInputInfo.name.c_str(), _modelId.c_str());
      return TERMINAL_ERROR;
    }
    if (req.inputs[clientInputIndex].dataType != PREPROCESSOR_INPUT_TYPE) {
      LOG_TO_ERROR(
          "For inputName=%s DataType=%d (interpreted as ModelInput), but should be of type "
          "UserInput",
          req.inputs[clientInputIndex].name, req.inputs[clientInputIndex].dataType);
      return TERMINAL_ERROR;
    }
    for (auto inputName : preprocessorInputInfo.inputNames) {
      int preprocessorInputIndex = -1;
      if (_inputNamesToIdMap.find(inputName) == _inputNamesToIdMap.end()) {
        LOG_TO_ERROR(
            "Id:%s Inference: inputName=%s does not exist for model %s given in "
            "preprocessorInput=%s",
            inferId.c_str(), inputName.c_str(), _modelId.c_str());
        return TERMINAL_ERROR;
      } else {
        preprocessorInputIndex = _inputNamesToIdMap[inputName];
      }
      if (!_info.inputs[preprocessorInputIndex].toPreprocess) {
        LOG_TO_ERROR(
            "Id:%s Inference: inputName=%s does not contain a preprocessor, for modelId=%s given "
            "in preprocessorInput=%s",
            inferId.c_str(), inputName.c_str(), _modelId.c_str());
        return TERMINAL_ERROR;
      }

      auto userInput = _commandCenter->get_userEventsManager()->get_model_input(
          _modelId + _info.inputs[preprocessorInputIndex].name,
          (SinglePreprocessorInput*)req.inputs[clientInputIndex].data,
          req.inputs[clientInputIndex].length);
      if (!userInput) {
        LOG_TO_ERROR("preprocessor feature input not valid for inputName=%s modelId=%s",
                     _info.inputs[preprocessorInputIndex].name.c_str(), _modelId.c_str());
        return TERMINAL_ERROR;
      }
      preprocessorInputsToFill.push_back(
          ONNXPreProcessorInput(userInput, &_info.inputs[preprocessorInputIndex]));
      if (!check_input(
              inferId, preprocessorInputIndex, _info.inputs[preprocessorInputIndex].dataType,
              userInput->length *
                  get_field_size_from_data_type(_info.inputs[preprocessorInputIndex].dataType))) {
        return TERMINAL_ERROR;
      }
      Ort::Value createdTensor = create_ort_tensor(preprocessorInputIndex, userInput->data);
      if (createdTensor.HasValue())
        _inputTensors[preprocessorInputIndex] = std::move(createdTensor);
      else
        return TERMINAL_ERROR;  // error already logged in create_ort_tensor
    }
  }
  auto stop = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
  nlohmann::json preprocessingMetric;
  preprocessingMetric["source"] = "preprocessor";
  preprocessingMetric["inferenceId"] = inferId;
  preprocessingMetric["uTime"] = duration.count();
  preprocessingMetric["modelId"] = _modelId;
  preprocessingMetric["modelVersion"] = _version;
  _commandCenter->log_metrics(INFERENCEMETRIC, preprocessingMetric);
  if (!allocate_output_memory(ret)) {
    return TERMINAL_ERROR;
  }
  if (_commandCenter->get_config().isDebug()) {
    for (int i = 0; i < _info.inputs.size(); i++) {
      auto shape = _inputTensors[i].GetTensorTypeAndShapeInfo().GetShape();
      std::string inputString = "";
      switch (_info.inputs[i].dataType) {
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT:
          inputString = recursive_string(shape, 0, _inputTensors[i].GetTensorData<float>(), 0,
                                         _info.inputs[i].size);
          break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32:
          inputString = recursive_string(shape, 0, _inputTensors[i].GetTensorData<int32_t>(), 0,
                                         _info.inputs[i].size);
          break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE:
          inputString = recursive_string(shape, 0, _inputTensors[i].GetTensorData<double>(), 0,
                                         _info.inputs[i].size);
          break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64:
          inputString = recursive_string(shape, 0, _inputTensors[i].GetTensorData<int64_t>(), 0,
                                         _info.inputs[i].size);
          break;
        default:
          break;
      }
      LOG_TO_CLIENT_DEBUG("CLIENTDEBUG: %s=%s", _info.inputs[i].name.c_str(), inputString.c_str());
    }
  }
  try {
    _session->Run(Ort::RunOptions{nullptr}, inputNames.data(), _inputTensors.data(),
                  inputNames.size(), ret->outputNames, _outputTensors.data(), ret->numOutputs);
  } catch (Ort::Exception e) {
    LOG_TO_ERROR("Exception in get_inference:%s with errorCode:%d, for modelId=%s", e.what(),
                 e.GetOrtErrorCode(), _modelId.c_str());
    deallocate_output_memory(ret);
    return TERMINAL_ERROR;
  } catch (...) {
    LOG_TO_ERROR("Exception in get_inference ONNXSessionRun for modelId=%s", _modelId.c_str());
    deallocate_output_memory(ret);
    return TERMINAL_ERROR;
  }
  return SUCCESS;
}

ONNXModel::ONNXModel(const ONNXModelInfo& modelInfo, const std::string& plan, Ort::Env& mEnv,
                     const std::string& version, const std::string& modelId,
                     CommandCenter* commandCenter)
    : _memoryInfo(Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator,
                                             OrtMemType::OrtMemTypeDefault)) {
  std::lock_guard<std::mutex> locker(_modelMutex);
  _commandCenter = commandCenter;
  _modelId = modelId;
  _version = version;
  _info = modelInfo;
  _sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
  _session = new Ort::Session(mEnv, plan.c_str(), plan.length(), _sessionOptions);
  for (int i = 0; i < _info.inputs.size(); i++) {
    if (_info.inputs[i].toPreprocess) {
      bool preprocessorCreated = false;
      switch (_info.inputs[i].dataType) {
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT:
          preprocessorCreated = _commandCenter->get_userEventsManager()->create_preprocessor<float>(
              _modelId + _info.inputs[i].name, _info.inputs[i].preprocessorJson);
          break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64:
          preprocessorCreated =
              _commandCenter->get_userEventsManager()->create_preprocessor<int64_t>(
                  _modelId + _info.inputs[i].name, _info.inputs[i].preprocessorJson);
          break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32:
          preprocessorCreated =
              _commandCenter->get_userEventsManager()->create_preprocessor<int32_t>(
                  _modelId + _info.inputs[i].name, _info.inputs[i].preprocessorJson);
          break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE:
          preprocessorCreated =
              _commandCenter->get_userEventsManager()->create_preprocessor<double>(
                  _modelId + _info.inputs[i].name, _info.inputs[i].preprocessorJson);
          break;
        default:
          LOG_TO_ERROR("Preprocessor not defined for type %d", _info.inputs[i].dataType);
          throw std::runtime_error("");
      }
      if (!preprocessorCreated) {
        LOG_TO_ERROR("Could not create preprocessor for inputName=%s, modelid=%s",
                     _info.inputs[i].name.c_str(), _modelId.c_str());
        throw std::runtime_error("");
      }
    }
    _inputTensors.push_back(Ort::Value(nullptr));
    _inputNamesToIdMap[_info.inputs[i].name] = i;
  }
  run_dummy_inference();
}

void delete_input_memory(void** input, int size) {
  for (int i = 0; i < size; i++) free(input[i]);
  free(input);
}

void ONNXModel::run_dummy_inference() {
  void** input;
  ONNXInferenceReturn ret;
  std::vector<const char*> inputNames;
  int numInputs = _info.inputs.size();
  input = (void**)malloc(sizeof(void*) * numInputs);
  for (int i = 0; i < _info.inputs.size(); i++) {
    inputNames.push_back(_info.inputs[i].name.c_str());
    input[i] = (void*)malloc(get_field_size_from_data_type(_info.inputs[i].dataType) *
                             _info.inputs[i].size);
    memset((char*)input[i], 0,
           get_field_size_from_data_type(_info.inputs[i].dataType) * _info.inputs[i].size);
    Ort::Value createdTensor = create_ort_tensor(i, input[i]);
    if (createdTensor.HasValue())
      _inputTensors[i] = std::move(createdTensor);
    else
      throw std::runtime_error("");  // error already logged in create_ort_tensor
  }
  if (!allocate_output_memory(&ret))
    throw std::runtime_error("");  // error already logged in create_ort_tensor

  try {
    _session->Run(Ort::RunOptions{nullptr}, inputNames.data(), _inputTensors.data(),
                  inputNames.size(), ret.outputNames, _outputTensors.data(), ret.numOutputs);
  } catch (Ort::Exception e) {
    LOG_TO_DEBUG(
        "Exception in running_dummy_inference:%s with errorCode:%d, for modelId=%s, version=%s",
        e.what(), e.GetOrtErrorCode(), _modelId.c_str(), _version.c_str());
  } catch (...) {
    LOG_TO_DEBUG("Exception in running_dummy_inference for modelId=%s, version=%s",
                 _modelId.c_str(), _version.c_str());
  }
  deallocate_output_memory(&ret);
  // deleting input memory
  delete_input_memory(input, numInputs);
}

ONNXModel::~ONNXModel() { delete _session; }

void deallocate_output_memory(InferenceReturn* ret) {
  for (int i = 0; i < ret->numOutputs; i++) {
    free(ret->outputs[i]);
    free(ret->outputShapes[i]);
  }
  free(ret->outputs);
  free(ret->outputNames);
  free(ret->outputTypes);
  free(ret->outputShapes);
  free(ret->outputLengths);
  free(ret->outputShapeLengths);
}

void ONNXModel::get_model_status(ModelStatus* status) {
  status->isModelReady = true;
  asprintf(&(status->version), "%s", _version.c_str());
}
