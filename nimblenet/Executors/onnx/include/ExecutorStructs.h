
#pragma once

struct ONNXInferenceReturn {
  void** outputs;
  int** outputShapes;
  int* outputLengths;
  int* outputShapeLengths;
  char** outputNames;
  int* outputTypes;
  int numOutputs;
};

struct ModelStatus {
  bool isModelReady;
  char* version;
};

typedef struct ONNXInferenceReturn InferenceReturn;

struct UserInput {
  void* data;
  int length;
  char* name;
  int dataType;
};

struct ONNXInferenceRequest {
  int numInputs;
  UserInput* inputs;
};

struct SinglePreprocessorInput {
  char** keys;
  char** values;
  int keysLength;
};

typedef struct ONNXInferenceRequest InferenceRequest;

void deallocate_output_memory(InferenceReturn* ret);
