#include <torch/script.h>

#include <map>
typedef torch::jit::script::Module* ModulePtr;

struct ModelInfo {
  ModulePtr modPtr = nullptr;
  bool modelLoaded = false;
};

class TorchExecutor {
  std::map<int, ModelInfo> modelMap;
  torch::jit::script::Module get_plan_from_string(const std::string& plan);
  c10::List<at::Tensor> get_tensors_from_model_string(const std::string& model);

 public:
  bool check_plan_loaded(int resourceId);
  bool check_model_loaded(int resourceId);
  bool load_model(const std::string& model, int resourceId);
  bool load_plan(const std::string& plan, int resourceId);
  bool load_model_and_plan(int resourceId, const std::string& model, const std::string& plan);
  bool delete_model_and_plan(int resourceId);
  float* get_inference(int resourceId, float* input, int* inputShape, int iShapeLen,
                       char* inference_logs) const;
};
