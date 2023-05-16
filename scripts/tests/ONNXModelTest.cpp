#include <iostream>

#include "Executor.h"  // Corresponding to ONNX
#include "NativeInterface.h"
#include "util.h"
using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << "Usage: " << argv[0] << " <ONNXModelFilePath> <ONNXPlanFeaturesFilePath>" << endl;
    exit(1);
  }
  logger.initLogger("./");
  string plan;
  if (!NativeInterface::get_file_from_device(argv[1], plan)) {
    cerr << "Could not read ONNXModelFile:" << argv[1] << endl;
    exit(1);
  }
  string planFeatures;
  if (!NativeInterface::get_file_from_device(argv[2], planFeatures)) {
    cerr << "Could not read ONNXPlanFeaturesFile:" << argv[1] << endl;
    exit(1);
  }
  string modelId1 = "testModel1";
  Ort::Env onxEnv = Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, "ONNX Environment");
  ONNXModelInfo modelInfo = JSONParser::get<ONNXModelInfo>(planFeatures);
  ONNXInferenceReturn ret;
  ONNXInferenceRequest req;

  int numInputs = modelInfo.inputs.size();
  req.inputs = new void *[numInputs];
  req.inputLengths = new int[numInputs];
  req.inputNames = new char *[numInputs];
  req.inputTypes = new int[numInputs];
  req.numInputs = numInputs;
  for (int i = 0; i < numInputs; i++) {
    modelInfo.inputs[i].isFeature = false;
    req.inputLengths[i] = modelInfo.inputs[i].size;
    req.inputNames[i] = const_cast<char *>(modelInfo.inputs[i].name.c_str());
    req.inputTypes[i] = modelInfo.inputs[i].dataType;
    req.inputs[i] =
        (void *)new char[ONNXModel::get_field_size_from_data_type(modelInfo.inputs[i].dataType) *
                         modelInfo.inputs[i].size];
  }

  // passing commandCenter as a nullptr as this should not be used
  ONNXModel model1(modelInfo, plan, onxEnv, "1.0.0", modelId1, nullptr);

  if (!model1.get_inference("run1", req, &ret)) {
    cerr << "Could not get inference" << endl;
    exit(1);
  }

  vector<bool> visitedOutput(modelInfo.outputs.size(), false);
  for (int i = 0; i < ret.numOutputs; i++) {
    cout << "Length of outputName:" << ret.outputNames[i] << " " << ret.outputLengths[i] << endl;
    bool outputIsCorrectlyFormed = false;
    for (int j = 0; j < modelInfo.outputs.size(); j++) {
      if (visitedOutput[j]) continue;
      if (modelInfo.outputs[j].name == ret.outputNames[i]) {
        visitedOutput[j] = true;
        if (!modelInfo.outputs[j].dataType == ret.outputTypes[i]) {
          cerr << "Output dataType does not match for outputName:" << ret.outputNames[i] << endl;
          exit(1);
        }
        if (!modelInfo.outputs[j].size == ret.outputLengths[i]) {
          cerr << "Output lengths does not match for outputName:" << ret.outputNames[i] << endl;
          exit(1);
        }
        if (!modelInfo.outputs[j].shape.size() == ret.outputShapeLengths[i]) {
          cerr << "Output shape lengths does not match for outputName:" << ret.outputNames[i]
               << endl;
          exit(1);
        }
        for (int k = 0; k < modelInfo.outputs[j].shape.size(); k++) {
          if (!(modelInfo.outputs[j].shape[k] == ret.outputShapes[i][k])) {
            cerr << "Output shapes do not match for outputName:" << ret.outputNames[i] << " i:" << i
                 << " j:" << j << " k:" << k << " expected:" << modelInfo.outputs[j].shape[k]
                 << " gotFromORT:" << ret.outputShapes[i][k] << endl;
            exit(1);
          }
        }
        outputIsCorrectlyFormed = true;
      }
    }
    if (!outputIsCorrectlyFormed) {
      cerr << "Check logs, one outputName:" << ret.outputNames[i] << " not found" << endl;
      exit(1);
    }
  }

  deallocate_output_memory(&ret);
}
