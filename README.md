# nimbleSDK
## Setup
Run: `./setup.sh`
This will download onnx for linux and place it in `libLinux/onnx/`

## Build
Run: `./build.sh`
This will run `cmake` and start compiling an executable `build/nimbleclient` from `main.cpp`.

## Functions exposed
`initialize_nimblenet` takes a string config as argument. This function will fetch the required models from server
and get ready for inference.
Sample Config:
```
{
  "client_id" : "client123", # unique id that we have given to a client(company) 
  "device_id" : "device2", # unique id that the client will have given to each device.
  "host" : "https://nimbleedge.ai", # url where are server is running
  "port" : 443, # port of our server
  "model_ids" : ["model1","model2"] # models that will be used later for inferences or otherwise
}
```
`initialize_inference` takes a modelName as input. This will take the lock, so that no other inference of the same model can be running. \n
`get_inference` takes a InferenceRequest struct as input. This will return a InferenceReturn object. You can see main.cpp on how to use this. \n
`exit_inference` takes a modelName as input. This will release the lock, so that other inferences of the same model can run. Before calling `exit_inference`,
you should ensure that you have copied the data from the output vectors as this will be overwritten during the next inference.
