add_library(clientlib SHARED src/client.cpp include/client.h)
target_link_libraries(clientlib log)
target_include_directories(clientlib PUBLIC "${PROJECT_SOURCE_DIR}/android/include/")
target_include_directories(clientlib PUBLIC "${PROJECT_SOURCE_DIR}/commonclient/include/")
