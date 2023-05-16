#include "client.h"

#include <android/log.h>

#include <string>

JavaVM *globalJvm;
std::string androidClass;
jobject context;

jmethodID sendRequest;
jclass sendRequestClazz;

jmethodID Save;
jclass saveClazz;

jmethodID Retrieve;
jclass retrieveClazz;

jmethodID getDynamicDeviceMetrics;
jclass hardwareInfoClazz;

jobject gClassLoader;
jmethodID gFindClassMethod;

jclass findClass(JNIEnv *env, const char *name) {
  jclass clazz = static_cast<jclass>(
      env->CallObjectMethod(gClassLoader, gFindClassMethod, env->NewStringUTF(name)));
  return clazz;
}

CNetworkResponse send_request(const char *body, const char *headers, const char *url,
                              const char *method) {
  // in the new thread:
  JNIEnv *env;
  bool attached = false;
  int getEnvStatus = globalJvm->GetEnv((void **)&env, JNI_VERSION_1_6);

  if (getEnvStatus == JNI_EDETACHED) {
    attached = true;
    JavaVMAttachArgs args;
    args.version = JNI_VERSION_1_6;  // choose your JNI version
    args.name = NULL;                // you might want to give the java thread a name
    args.group = NULL;               // you might want to assign the java thread to a ThreadGroup

    if (globalJvm->AttachCurrentThread(&env, NULL) != 0) {
      return emptyResponse();
    }

    sendRequestClazz = findClass(env, "ai/nimbleedge/io/Networking");
  } else {
    sendRequestClazz = env->FindClass("ai/nimbleedge/io/Networking");
  }

  sendRequest =
      env->GetStaticMethodID(sendRequestClazz, "sendRequest",
                             "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/"
                             "String;)Lai/nimbleedge/datamodels/NetworkResponse;");

  // And now you can use myNewEnv

  jstring jurl = env->NewStringUTF(url);
  jstring jheaders = env->NewStringUTF(headers);
  jstring jbody = env->NewStringUTF(body);
  jstring jmethod = env->NewStringUTF(method);

  jobject result =
      env->CallStaticObjectMethod(sendRequestClazz, sendRequest, jurl, jheaders, jbody, jmethod);

  jclass resultClass = env->GetObjectClass(result);

  CNetworkResponse response{};

  response.statusCode =
      (int)env->GetIntField(result, env->GetFieldID(resultClass, "statusCode", "I"));

  auto responseHeaders = (jstring)env->GetObjectField(
      result, env->GetFieldID(resultClass, "headers", "Ljava/lang/String;"));
  char *tempHeaders = const_cast<char *>(env->GetStringUTFChars(responseHeaders, nullptr));
  response.headers = (char *)malloc(strlen(tempHeaders));
  memcpy(response.headers, tempHeaders, strlen(tempHeaders));
  env->ReleaseStringUTFChars(responseHeaders, tempHeaders);
  auto responseBody =
      (jbyteArray)env->GetObjectField(result, env->GetFieldID(resultClass, "body", "[B"));
  char *temp = (char *)(env->GetByteArrayElements(responseBody, nullptr));
  response.bodyLength =
      (int)env->GetIntField(result, env->GetFieldID(resultClass, "bodyLength", "I"));
  response.body = (char *)malloc(response.bodyLength);
  memcpy(response.body, temp, response.bodyLength);
  env->ReleaseByteArrayElements(responseBody, (jbyte *)temp, 0);
  if (attached) {
    globalJvm->DetachCurrentThread();
  }
  return response;
}

char *get_battery_status() { return nullptr; }

char *get_temperature_status() { return nullptr; }

char *get_cpu_status() { return nullptr; }

char *get_memory_status() { return nullptr; }

char *get_hardware_info() { return nullptr; }

char *save(const char *inputStream, const char *filePath, int overwrite) {
  JNIEnv *env;
  bool attached = false;
  int getEnvStatus = globalJvm->GetEnv((void **)&env, JNI_VERSION_1_6);
  if (getEnvStatus == JNI_EDETACHED) {
    // log_debug("THREAD ATTACHED TO JNI");
    attached = true;
    JavaVMAttachArgs args;
    args.version = JNI_VERSION_1_6;  // choose your JNI version
    args.name = NULL;                // you might want to give the java thread a name
    args.group = NULL;               // you might want to assign the java thread to a ThreadGroup
    if (globalJvm->AttachCurrentThread(&env, NULL) != 0) {
      // std::cout << "Failed to attach" << std::endl;
      return "";
    }
    saveClazz = findClass(env, "ai/nimbleedge/io/IO");
  } else {
    saveClazz = env->FindClass("ai/nimbleedge/io/IO");
    Save = env->GetStaticMethodID(
        saveClazz, "save",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
  }

  jstring jinput = env->NewStringUTF(inputStream);
  jstring jfilepath = env->NewStringUTF(filePath);
  jobject result = env->CallStaticObjectMethod(saveClazz, Save, context, jinput, jfilepath);
  const char *str = env->GetStringUTFChars((jstring)result, NULL);
  if (attached) {
    globalJvm->DetachCurrentThread();
  }
  return const_cast<char *>(str);
}

int deleteData(const char *filePath) { return 1; }

char *retrieve(const char *filePath) {
  JNIEnv *env;
  bool attached = false;
  int getEnvStatus = globalJvm->GetEnv((void **)&env, JNI_VERSION_1_6);
  if (getEnvStatus == JNI_EDETACHED) {
    // log_debug("THREAD ATTACHED TO JNI");
    attached = true;
    JavaVMAttachArgs args;
    args.version = JNI_VERSION_1_6;  // choose your JNI version
    args.name = NULL;                // you might want to give the java thread a name
    args.group = NULL;               // you might want to assign the java thread to a ThreadGroup
    if (globalJvm->AttachCurrentThread(&env, NULL) != 0) {
      // std::cout << "Failed to attach" << std::endl;
      return "";
    }
    retrieveClazz = findClass(env, "ai/nimbleedge/io/IO");
  } else {
    retrieveClazz = env->FindClass("ai/nimbleedge/io/IO");
    Retrieve =
        env->GetStaticMethodID(retrieveClazz, "retrieve",
                               "(Landroid/content/Context;Ljava/lang/String;)Ljava/lang/String;");
  }

  jstring jfilepath = env->NewStringUTF(filePath);
  jobject result = env->CallStaticObjectMethod(retrieveClazz, Retrieve, context, jfilepath);
  const char *str = env->GetStringUTFChars((jstring)result, NULL);
  if (attached) {
    globalJvm->DetachCurrentThread();
  }
  return const_cast<char *>(str);
}

int check_if_exists(const char *filePath) { return 1; }

void log_debug(const char *message) {
  __android_log_print(ANDROID_LOG_DEBUG, "NIMBLE-CORE", "%s", message);
}

void log_info(const char *message) {
  __android_log_print(ANDROID_LOG_INFO, "NIMBLE-CORE", "%s", message);
}

void log_warn(const char *message) {
  __android_log_print(ANDROID_LOG_WARN, "NIMBLE-CORE", "%s", message);
}

void log_error(const char *message) {
  __android_log_print(ANDROID_LOG_ERROR, "NIMBLE-CORE", "%s", message);
}

void log_fatal(const char *message) {
  __android_log_print(ANDROID_LOG_FATAL, "NIMBLE-CORE", "%s", message);
}

void init_logger(const char *deviceId, const char *apiKey) {}

void send_pending_logs() {}

char *save_to_cache(const char *data, const char *filePath) { return nullptr; }

char *retrieve_from_cache(const char *filePath) { return nullptr; }

CNetworkResponse emptyResponse() {
  CNetworkResponse response{};
  response.statusCode = 900;
  response.headers = nullptr;
  response.body = nullptr;
  return response;
}

char *get_metrics(const char *metricType) {
  JNIEnv *env;
  bool attached = false;
  int getEnvStatus = globalJvm->GetEnv((void **)&env, JNI_VERSION_1_6);
  if (getEnvStatus == JNI_EDETACHED) {
    // log_debug("THREAD ATTACHED TO JNI");
    attached = true;
    if (globalJvm->AttachCurrentThread(&env, NULL) != 0) {
      // std::cout << "Failed to attach" << std::endl;
      return nullptr;
    }
    hardwareInfoClazz = findClass(env, "ai/nimbleedge/io/HardwareInfo");
  } else {
    hardwareInfoClazz = env->FindClass("ai/nimbleedge/io/HardwareInfo");
  }

  getDynamicDeviceMetrics = env->GetStaticMethodID(hardwareInfoClazz, "getDynamicDeviceMetrics",
                                                   "(Landroid/content/Context;)Ljava/lang/String;");
  jobject result = env->CallStaticObjectMethod(hardwareInfoClazz, getDynamicDeviceMetrics, context);
  const char *str = env->GetStringUTFChars((jstring)result, NULL);
  auto strCopy = (char *)malloc(strlen(str) + 1);
  memcpy(strCopy, str, strlen(str));
  strCopy[strlen(str)] = 0;
  env->ReleaseStringUTFChars((jstring)result, str);
  if (attached) {
    globalJvm->DetachCurrentThread();
  }
  return const_cast<char *>(strCopy);
}
