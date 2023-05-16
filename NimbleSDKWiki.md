# Nimble SDK 
This contains the design for the core SDK deployed on all devices, responsible for all training and inference that happens on device. It is written in C++ to be cross platform and compatible with most hardware, while being lightning fast and small.  

## Code Structure 
Important Classes and Locations: 

nimblenet/nimblenet.cpp - Functions exposed here can be called from the wrapper SDKs, written on top of nimbleSDK. Both nimblenet.h and nimblenet.cpp follow C only syntax to be comptabile with iOS since it supports C only syntax for the Swift integration. 

CommandCenter.cpp - Entry point for most of the things exposed in nimblenet.cpp. Kind of the manager for all things nimbleSDK. 

ResourceManager.cpp - Manages lifecycle of the Inference/Training resources needed on the device to perform the functions (i.e Inference/Training/Data Processing) 

ServerAPI.cpp - All network interactions from the SDK needs to happen through this static class. It uses Native Interface written based on different supported platforms to make network calls and return the data in format understandable to the rest of nimbleSDK. 

ONNXExecutor.cpp - Machine Learning component of nimbleSDK. Present as _mlExecutor in CommandCenter. All other executors(Pytorch/TF-Lite) etc needs to be written in the same way and exposed via _mlExecutor.

FeatureStore.cpp - Feature Store Manager, which is responsible to download and update Feature Store on each device. Feature Stores are consumed in as inputs for different models, to perform better inferences based on current state of non user related data. 

NativeInterface.cpp - Acts as an interface for all the things we need to perform on different platforms, for ex network Calls, read/write on disk, get device related metrics and all. 

Client.h / Client.cpp - Platform specific code. NativeInteface uses these two based on platform for which it is built in order to peform different actions.

ConfigManager.cpp - Loads the config required for nimbleSDK to function. SDK needs to be aware of which models to load and use, where to send logs and other parameters.

_cmdThread and perform_long_running_tasks() - Detached thread from nimbleSDK, which is used for all the tasks that need to performed asyncronously on the device(download of components/sending of logs/syncing of components)


## Control Flow 

1. Wrapper SDK(Android/iOS) loads nimbleSDK  and Initializes it by passing Config as well as a directory location where SDK can write its log.
2. nimblenet.cpp has one instance of CommandCenter object, which ensures, that everything flows serially into the commandCenter. We ensure there is no RACE for init as well as other tasks manually. 
3. CommandCenter pareses the config and inits all the required Classes. 
4. It also looks for any previously saved ML objects present on the device, loads it in memory.
CommandCenter initializes the _cmdThread with the method, which then keeps on trying to download required ML object, load it in memory, get ready for ML operations on the device. 
5. Once initialized, wrapper SDK can call all other Methods of nimbleSDK to get the desired results.


## Considerations 

Both nimblenet.h and nimblenet.cpp follow C only syntax to be comptabile with iOS since it supports C only syntax for the Swift integration. 

Init, Get, Exit Inference -  We expose 3 methods for inference. This leads to us exposing the lock to the wrapper SDK, which is anti pattern. However, there was no way to ensure the wrapper SDK has read the Inference output we provided. In case of a multithreaded GetInference requests, if we released lock as soon as we Complete inference, we can not guarantee the client has received the information, since it might be the case that we started other inference and overwrote the values in the memory.   

Also, we dont ask the wrapper SDK to provide us with the memory on which we can write the Inference results, since wrapper SDK is not aware of the Model output sizes, and thus cannot allocate memory in advance for that. 

We do NOT call Device Register on every app start, so we cannot perform any action inside Device Register which needs to happen everytime the app opens.