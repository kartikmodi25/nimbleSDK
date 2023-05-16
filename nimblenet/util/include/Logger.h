#pragma once
#include <chrono>
#include <ctime>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "client.h"

#define LOG_TIMER_INTERVAL_IN_SECONDS 5
#define DEFAULT_UPLOAD_LOGS_URL "https://logs.nimbleedgehq.ai"

#if defined(__ANDROID__)
#define PLATFORM "android"
#else
#define PLATFORM "linux"
#endif

struct LoggingConfig {
  std::string _host = DEFAULT_UPLOAD_LOGS_URL;
  std::string _secretKey;
  std::string _deviceId;
  std::string _clientId;
  const std::string _service = "nimbleSDK";
  const std::string _source = PLATFORM;
  const std::string _tags = "env:nimblepoc,version:1.0";
  char* _defaultSecretKey;

  LoggingConfig() {
    std::vector<int64_t> secVec = {3617574009957856822, 7161680211933160759, 3834033765364414521,
                                   7378366457403629875};
    _defaultSecretKey = new char[secVec.size() * 8 + 1];
    int64_t* d1 = (int64_t*)_defaultSecretKey;
    for (int i = 0; i < secVec.size(); i++) {
      *d1 = secVec[i];
      ++d1;
    }
    _defaultSecretKey[secVec.size() * 8] = 0;
  }
};

class Logger {
  std::string _writeFile;
  std::string _logDirectory;
  FILE* _writeFilePtr;
  LoggingConfig _log_config;
  std::string _logLocation;
  std::mutex _logMutex;
  std::chrono::high_resolution_clock::time_point _lastLogTime;
  std::chrono::seconds _timer_interval = std::chrono::seconds(LOG_TIMER_INTERVAL_IN_SECONDS);
  bool _isDebug = false;

 public:
  bool init_logger(const std::string& logDir);
  void update_device_details(const std::string& deviceId, const std::string& clientId,
                             bool debug = false);
  void write_log(const char* message, const char* type);
  bool send_logs(const std::vector<std::string>& processed_logs, const std::string& logfilePath);
  void send_pending_logs(bool initComplete);
  void update_log_config(const std::string& secretKey, int time_interval,
                         const std::string& logUrl = DEFAULT_UPLOAD_LOGS_URL);

  template <typename... Args>
  char* formatlog(const char* format, Args... args) {
    int size_s = std::snprintf(nullptr, 0, format, args...) + 1;  // Extra space for '\0'
    if (size_s <= 0) {
      throw std::runtime_error("Error during formatting.");
    }
    auto size = static_cast<size_t>(size_s);
    char* buf = new char[size];
    std::snprintf(buf, size, format, args...);
    return buf;
  }

  template <typename... Args>
  void LOGDEBUG(const char* format, Args... args) {
    char* buf = formatlog(format, args...);
    write_log(buf, "DEBUG:::");
#ifdef NDEBUG
    // non debug
#else
    log_debug(buf);
#endif
    delete[] (buf);
  }

  template <typename... Args>
  void LOGINFO(const char* format, Args... args) {
    char* buf = formatlog(format, args...);
    write_log(buf, "INFO:::");
    log_info(buf);
    delete[] (buf);
  }

  template <typename... Args>
  void LOGWARN(const char* format, Args... args) {
    char* buf = formatlog(format, args...);
    write_log(buf, "WARN:::");
    log_warn(buf);
    delete[] (buf);
  }

  template <typename... Args>
  void LOGERROR(const char* format, Args... args) {
    char* buf = formatlog(format, args...);
    write_log(buf, "ERROR:::");
    log_error(buf);
    delete[] (buf);
  };

  template <typename... Args>
  void LOGFATAL(const char* format, Args... args) {
    char* buf = formatlog(format, args...);
    write_log(buf, "FATAL:::");
    log_fatal(buf);
    delete[] (buf);
  }

  template <typename... Args>
  void LOGMETRICS(const char* metricType, const char* metricJsonString) {
    char* buf = formatlog("%s ::: %s", metricType, metricJsonString);
    write_log(buf, "METRICS:::");
    delete[] (buf);
  }

  template <typename... Args>
  void OUTSIDELOG(const char* format, Args... args) {
    char* buf = formatlog(format, args...);
    write_log(buf, "OUTSIDE:::");
    delete[] (buf);
  }

  template <typename... Args>
  void CLIENTDEBUGLOG(const char* format, Args... args) {
    if (!_isDebug) {
      return;
    }
    char* buf = formatlog(format, args...);
    log_debug(buf);
    delete[] (buf);
  }
};

extern Logger logger;
#define LOG_TO_ERROR(fmt, ...) logger.LOGERROR(fmt, ##__VA_ARGS__)
#define LOG_TO_INFO(fmt, ...) logger.LOGINFO(fmt, ##__VA_ARGS__)
#define LOG_TO_FATAL(fmt, ...) logger.LOGFATAL(fmt, ##__VA_ARGS__)
#define LOG_TO_WARN(fmt, ...) logger.LOGWARN(fmt, ##__VA_ARGS__)
#define LOG_TO_DEBUG(fmt, ...) logger.LOGDEBUG(fmt, ##__VA_ARGS__)
#define LOG_TO_CLIENT_DEBUG(fmt, ...) logger.CLIENTDEBUGLOG(fmt, ##__VA_ARGS__)
