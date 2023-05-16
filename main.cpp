#include <unistd.h>

#include <iostream>
#include <vector>

#include "crossPlatformUtil.h"
#include "nimblenet.h"
#include "string.h"
#include "time.h"
using namespace std;

int main() {
  // ftell(nullptr);
  cout << initialize_nimblenet(
              R"delim(
		{
			
			"clientID": "testclient",
			"modelIDs": ["D11LightGBMModel"],
			"host": "https://api.nimbleedge-staging.com",
			"clientSecret": "samplekey123",
      "debug": true,
      "maxInputsToSave": 1,
			"databaseConfig" : [
						{
							"tableName": "UserClicks",
							"schema": {
								"contestType": "TEXT",
								"productid" : "INT",
								"roundid" : "INT",
								"winnerPercent" : "INT",
								"prizeAmount": "REAL",
								"entryFee": "INT"
							      },
							"expiryInMins":60
						}
					 ]
		}
	)delim",
              "./")
       << endl;
  usleep(5000000);
  internet_switched_on();
  add_event(
      R"({"contestType": "simple", "productid": 1, "roundid": 2, "winnerPercent":"012345", "prizeAmount": 0.0123, "entryFee" : 200})",
      "UserClicks");

  float input[] = {1.0, 2.0, 3.0, 4.0};
  float input2[] = {1.0, 2.0, 3.0, 4.0, 5.0};
  const char* modelId = "simplePlan";
  const char* modelId2 = "D11LightGBMModel";
  InferenceReturn ret;

  InferenceRequest req2;
  req2.numInputs = 2;
  req2.inputs = new UserInput[2];

  // std::vector<double> d(90, 0);
  UserInput inp1;
  // inp1.data = (void*)&d[0];
  // inp1.length = 90;
  // inp1.name = "input1";
  // inp1.dataType = 11;
  // req2.inputs[0] = inp1;
  // inp1.data = (void*)&d[0];
  // inp1.length = 90;
  // inp1.name = "input2";
  // req2.inputs[1] = inp1;
  // inp1.dataType = 11;
  // inp1.data = (void*)&d[0];
  // inp1.length = 90;
  // inp1.name = "input3";
  // req2.inputs[2] = inp1;
  // inp1.dataType = 11;
  // inp1.data = (void*)&d[0];
  // inp1.length = 90;
  // inp1.name = "input4";
  // req2.inputs[3] = inp1;
  // inp1.dataType = 11;
  // inp1.data = (void*)&d[0];
  // inp1.length = 90;
  // inp1.name = "input5";
  // req2.inputs[4] = inp1;
  // inp1.dataType = 11;
  std::vector<double> p(330, 0);
  inp1.data = (void*)&p[0];
  inp1.length = 330;
  inp1.name = "inputsToModel";
  inp1.dataType = 11;
  req2.inputs[0] = inp1;

  // req2.inputs[0] = inp1;

  UserInput inp2;
  SinglePreprocessorInput* products = new SinglePreprocessorInput[30];
  inp2.data = (void*)products;
  for (int i = 0; i < 30; i++) {
    products[i].keys = new char*[7];
    products[i].keys[0] = "contestType";
    products[i].keys[1] = "productid";
    products[i].keys[2] = "roundid";
    products[i].keys[3] = "winnerPercent";
    products[i].keys[4] = "prizeAmount";
    products[i].keys[5] = "entryFee";
    products[i].keys[6] = "acxjkoacmd";

    products[i].values = new char*[7];
    products[i].values[0] = "special";
    products[i].values[1] = "13529";
    products[i].values[2] = "97";
    products[i].values[3] = "20";
    products[i].values[4] = "2000000";
    products[i].values[5] = "50";
    products[i].values[6] = "vfdvbdcfv";

    products[i].keysLength = 7;
  }
  inp2.length = 30;
  inp2.name = "inputToProcessor";
  inp2.dataType = 666;
  req2.inputs[1] = inp2;

  for (int i = 0; i < 1; i++) {
    if (get_inference(modelId2, req2, &ret, "id2") != 200) continue;

    for (int q = 0; q < ret.numOutputs; q++) {
      cout << "Length of outputs " << ret.outputLengths[q] << endl;
      cout << "OUTPUT1:[";
      for (int j = 0; j < ret.outputLengths[q]; j++) {
        if (ret.outputTypes[q] == 1)
          cout << ((float*)ret.outputs[q])[j] << " ";
        else if (ret.outputTypes[q] == 7)
          cout << ((int64_t*)ret.outputs[q])[j] << " ";
      }
      cout << "] Output Shape:[";
      for (int j = 0; j < ret.outputShapeLengths[q]; j++) {
        cout << ret.outputShapes[q][j] << " ";
      }
      cout << "] " << endl;
    }
    exit_inference(modelId2, "id2", true);

    deallocate_output_memory(&ret);
    usleep(5000);
  }
  for (int i = 0; i < 2; i++) {
    cout << " GetInputsStatus  " << get_inputs_for_inferId("id2", &ret) << endl;
    for (int q = 0; q < ret.numOutputs; q++) {
      cout << "Length of inputs " << ret.outputLengths[q] << endl;
      cout << "INPUT:[";
      for (int j = 0; j < ret.outputLengths[q]; j++) {
        if (ret.outputTypes[q] == 1)
          cout << ((float*)ret.outputs[q])[j] << " ";
        else if (ret.outputTypes[q] == 7)
          cout << ((int64_t*)ret.outputs[q])[j] << " ";
        else if (ret.outputTypes[q] == 11)
          cout << ((double*)ret.outputs[q])[j] << " ";
      }
      cout << "] Input Shape:[";
      for (int j = 0; j < ret.outputShapeLengths[q]; j++) {
        cout << ret.outputShapes[q][j] << " ";
      }
      cout << "] " << endl;
    }
    deallocate_output_memory(&ret);
  }
}
