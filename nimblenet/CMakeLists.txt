set(BASE ServerAPI/src/ServerAPI.cpp 
	 ServerAPI/include/ServerAPI.h
	 ServerAPI/include/ServerAPIStructs.h
	 ServerAPI/src/ServerAPIStructs.cpp
	 ResourceManager/src/ResourceManager.cpp
	 ResourceManager/include/ResourceManager.h
	 ResourceManager/src/ResourceManagerStructs.cpp
	 ResourceManager/include/ResourceManagerStructs.h
	 ConfigManager/src/ConfigManager.cpp
	 ConfigManager/include/ConfigManager.h
	 nimblenet/include/nimblenet.h
	 nimblenet/include/crossPlatformUtil.h
	 nimblenet/src/nimblenet.cpp
	 CommandCenter/src/CommandCenter.cpp
	 CommandCenter/include/CommandCenter.h
	 NativeInterface/src/NativeInterface.cpp
	 NativeInterface/include/NativeInterface.h
	 NativeInterface/include/NativeInterfaceStructs.h
	 FeatureStore/include/FeatureStore.h 
	 FeatureStore/src/FeatureStore.cpp
	 UserEvents/AggregateColumn/src/CountColumn.cpp
	 UserEvents/AggregateColumn/src/SumColumn.cpp
	 UserEvents/AggregateColumn/src/AverageColumn.cpp
	 UserEvents/AggregateColumn/src/MinColumn.cpp
	 UserEvents/AggregateColumn/src/MaxColumn.cpp
	 UserEvents/PreProcessor/include/PreProcessor.h
	 UserEvents/PreProcessor/src/PreProcessor.cpp
	 UserEvents/TableStore/include/TableStore.h
	 UserEvents/TableStore/src/TableStore.cpp
	 UserEvents/RollingWindow/include/TimeBasedRollingWindow.h
	 UserEvents/RollingWindow/src/TimeBasedRollingWindow.cpp
	 UserEvents/RollingWindow/include/RollingWindow.h
	 UserEvents/RollingWindow/src/RollingWindow.cpp
	 UserEvents/UserEventsManager/include/UserEventsManager.h
	 UserEvents/UserEventsManager/src/UserEventsManager.cpp
	 UserEvents/UserEventsManager/src/UserEventsStruct.cpp
	 util/include/util.h
	 util/include/Logger.h
	 util/include/onnx.h
	 util/include/nlohmann_json.h
	 util/src/json.cpp
	 util/src/base64.cpp
	 util/src/Logger.cpp)
set(INCLUDES  "${PROJECT_SOURCE_DIR}/nimblenet/ServerAPI/include/"
					    "${PROJECT_SOURCE_DIR}/nimblenet/ResourceManager/include/"
					    "${PROJECT_SOURCE_DIR}/nimblenet/CommandCenter/include/"
					    "${PROJECT_SOURCE_DIR}/nimblenet/ConfigManager/include/"
					    "${PROJECT_SOURCE_DIR}/nimblenet/Logger/include/"
					    "${PROJECT_SOURCE_DIR}/nimblenet/NativeInterface/include/"
					    "${PROJECT_SOURCE_DIR}/nimblenet/util/include/"
						"${PROJECT_SOURCE_DIR}/nimblenet/FeatureStore/include/"
						"${PROJECT_SOURCE_DIR}/nimblenet/UserEvents/AggregateColumn/include/"
						"${PROJECT_SOURCE_DIR}/nimblenet/UserEvents/PreProcessor/include/"
						"${PROJECT_SOURCE_DIR}/nimblenet/UserEvents/RollingWindow/include/"
						"${PROJECT_SOURCE_DIR}/nimblenet/UserEvents/UserEventsManager/include/"
						"${PROJECT_SOURCE_DIR}/nimblenet/UserEvents/TableStore/include/"
					    )
if(ANDROID_ABI)
	message("USING ANDROID_ABI")
	list(APPEND BASE ../android/src/client.cpp)
	list(APPEND BASE ../android/include/client.h)
endif()
if(TORCH_EXECUTOR)
        add_definitions( -DTORCH_EXECUTOR )
	list(APPEND BASE Executors/pytorch/include/Executor.h
	                 Executors/pytorch/src/pytorchTrainer.cpp)
else()
        add_definitions( -DONNX_EXECUTOR )
	list(APPEND BASE Executors/onnx/include/Executor.h
			 Executors/onnx/include/ONNXExecutorStructs.h
			 Executors/onnx/src/ONNXExecutorStructs.cpp
			 Executors/onnx/src/ONNXExecutor.cpp)
endif()

add_library(nimblenet SHARED ${BASE})
if(ANDROID_ABI)
	target_include_directories(nimblenet PUBLIC "${PROJECT_SOURCE_DIR}/android/include/")
endif()
target_include_directories(nimblenet PRIVATE ${INCLUDES})
target_include_directories(nimblenet PUBLIC "${PROJECT_SOURCE_DIR}/commonclient/include/")
target_include_directories(nimblenet PUBLIC "${PROJECT_SOURCE_DIR}/nimblenet/nimblenet/include/")

if(PYTORCH_EXECUTOR)
	target_include_directories(nimblenet PUBLIC "${PROJECT_SOURCE_DIR}/nimblenet/Executors/pytorch/include/")
else()
	target_include_directories(nimblenet PUBLIC "${PROJECT_SOURCE_DIR}/nimblenet/Executors/onnx/include/")
endif()

