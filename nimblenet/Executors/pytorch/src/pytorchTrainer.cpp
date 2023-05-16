#include "Executor.h"
#include "util.h"
typedef torch::jit::script::Module ModuleType;

c10::List<at::Tensor> TorchExecutor::get_tensors_from_model_string(const std::string& model) {
  std::vector<std::vector<int64_t>> model_tensor_shape;
  std::vector<at::Tensor> model_tensors;

  std::string line, word;

  std::stringstream s(model);
  int count = 0;
  while (getline(s, line)) {
    std::stringstream line_stream(line);

    std::vector<int64_t> shape;
    std::vector<float> modelarr;

    while (getline(line_stream, word, ',')) {
      if (count % 2 == 0) {
        shape.push_back(std::stoll(word));
      } else {
        modelarr.push_back(std::stof(word));
      }
    }

    if (count % 2 == 0) {
      model_tensor_shape.push_back(shape);
    } else {
      float* model_data = &modelarr[0];
      auto tf = c10::IntArrayRef(model_tensor_shape[count / 2]);
      auto tensor = torch::from_blob(model_data, tf, torch::TensorOptions().dtype(torch::kFloat32));
      model_tensors.push_back(tensor.clone());
    }
    count++;
  }
  auto tensor_array_ref = c10::ArrayRef<at::Tensor>(model_tensors);
  return c10::List<at::Tensor>(tensor_array_ref);
}

torch::jit::script::Module TorchExecutor::get_plan_from_string(const std::string& plan) {
  std::istringstream is(plan);
  return torch::jit::load(is);
}

bool TorchExecutor::load_model(const std::string& model, int resourceId) {
  auto it = modelMap.find(resourceId);
  if (it != modelMap.end()) {
    ModelInfo& modInfo = it->second;
    auto tensor_list = get_tensors_from_model_string(model);
    modInfo.modPtr->run_method("set_model_params", tensor_list);
    modInfo.modelLoaded = true;
    return true;
  } else
    return false;  // plan to be loaded before loading model
}

bool TorchExecutor::load_plan(const std::string& plan, int resourceId) {
  auto it = modelMap.find(resourceId);
  if (it != modelMap.end()) {
    ModelInfo& modInfo = it->second;
    delete modInfo.modPtr;
    modInfo.modPtr = new ModuleType(get_plan_from_string(plan));
    modInfo.modelLoaded = false;
  }
  modelMap[resourceId] = ModelInfo();
  modelMap[resourceId].modPtr = new ModuleType(get_plan_from_string(plan));
  return true;
}

bool TorchExecutor::load_model_and_plan(int resourceId, const std::string& model,
                                        const std::string& plan) {
  return load_plan(plan, resourceId) && load_model(model, resourceId);
}

bool TorchExecutor::delete_model_and_plan(int resourceId) {
  auto it = modelMap.find(resourceId);
  if (it != modelMap.end()) {
    ModelInfo& modInfo = it->second;
    delete modInfo.modPtr;
    modelMap.erase(it);
  }
  return true;
}

float* TorchExecutor::get_inference(int resourceId, float* input, int* inputShape, int iShapeLen,
                                    char* inference_logs) const {
  std::string inference_logs_cpp = "";
  auto it = modelMap.find(resourceId);
  if (it == modelMap.end()) {
    return nullptr;
  }
  const ModelInfo& modInfo = it->second;
  if (!modInfo.modelLoaded) return nullptr;

  ModuleType& mod = *modInfo.modPtr;

  std::vector<int64_t> iShape(iShapeLen);
  for (int i = 0; i < iShapeLen; i++) iShape[i] = inputShape[i];
  auto tensor = torch::from_blob(input, c10::IntArrayRef(iShape),
                                 torch::TensorOptions().dtype(torch::kFloat32));

  auto outputTensor = mod.forward({tensor}).toTensor();
  // std::cout << "Forward pass done" << std::endl;
  outputTensor = outputTensor.squeeze(0).contiguous();
  auto len = outputTensor.size(0);
  float* preds = (float*)malloc(len * sizeof(float));
  // std::cout << "preds allocated" << std::endl;
  std::memcpy(preds, outputTensor.data_ptr<float>(), len * sizeof(float));
  // for(int i=0;i<len;i++)
  //	    std::cout<<preds[i]<<" ";
  //  std::cout<<std::endl;
  // std::cout << "Output tensor copied to preds" << std::endl;
  inference_logs_cpp.append("\0");
  strcpy(inference_logs, inference_logs_cpp.c_str());
  // std::cout << "Infernce code completed in C++" << std::endl;
  return preds;
}

bool TorchExecutor::check_model_loaded(int resourceId) {
  auto it = modelMap.find(resourceId);
  if (it != modelMap.end()) {
    ModelInfo& modInfo = it->second;
    return modInfo.modelLoaded;
  } else
    return false;
}

bool TorchExecutor::check_plan_loaded(int resourceId) {
  auto it = modelMap.find(resourceId);
  if (it != modelMap.end()) {
    return true;
  } else
    return false;
}
