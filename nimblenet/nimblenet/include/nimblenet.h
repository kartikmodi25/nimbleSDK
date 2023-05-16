#include "ExecutorStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

int initialize_nimblenet(const char* configJson, const char* homeDirectory);
int get_inference(const char* modelId, const InferenceRequest req, InferenceReturn* ret,
                  const char* inferId);
void exit_inference(const char* modelId, const char* inferId, bool success);
void write_log(const char* log);
void write_metric(const char* metricType, const char* metricJson);
bool add_event(const char* eventMapJsonString, const char* tableName);
bool get_model_status(const char* modelId, ModelStatus* status);
void internet_switched_on();
bool get_inputs_for_inferId(const char* inferId, ONNXInferenceReturn* ret);

#ifdef __cplusplus
}
#endif
