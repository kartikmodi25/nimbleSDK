#include "Logger.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <chrono>
#include <nlohmann/json.hpp>

#include "ConfigManager.h"
#include "NativeInterface.h"
#include "ServerAPI.h"
#include "client.h"
using json = nlohmann::json;

static std::string getNewFileToSendLog(const std::string& logDirectory) {
  std::string logFileName = "";
  DIR* logDir;
  struct dirent* dirContent;
  logDir = opendir(logDirectory.c_str());
  if (logDir != NULL) {
    while ((dirContent = readdir(logDir)) != NULL) {
      if (dirContent->d_type == DT_REG && strcmp(dirContent->d_name, "latest.txt")) {
        logFileName = std::string(dirContent->d_name);
        break;
      }
    }
    closedir(logDir);
  } else {
    LOG_TO_ERROR("Cannot open the directory %s to read logs", logDirectory.c_str());
  }
  if (logFileName == "") return logFileName;
  return logDirectory + "/" + logFileName;
}

std::string getStringDate() {
  timeval curTime;
  gettimeofday(&curTime, NULL);
  int milli = curTime.tv_usec / 1000;

  time_t rawtime;
  struct tm* timeinfo;
  char buffer[80];

  time(&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);

  char currentTime[84] = "";
  snprintf(currentTime, 84, "%s:%d", buffer, milli);
  return currentTime;
}

void Logger::write_log(const char* message, const char* type) {
  if (_writeFilePtr == NULL) return;
  std::lock_guard<std::mutex> locker(_logMutex);
  // Log type should always come first
  fprintf(_writeFilePtr, "%s %s ::: %s\n", type, getStringDate().c_str(), message);
  fflush(_writeFilePtr);
  long size = ftell(_writeFilePtr);
  if ((float)size / (MAX_BYTES_IN_KB) > MAX_LOG_FILE_SIZE_IN_KB) {
    fclose(_writeFilePtr);
    std::string newFileName = _logDirectory + "/log" + getStringDate() + ".txt";
    rename(_writeFile.c_str(), newFileName.c_str());
    _writeFilePtr = fopen(_writeFile.c_str(), "a+");
  }
}

std::vector<std::string> processLogs(const std::string& str) {
  std::vector<std::string> tokens;
  std::stringstream ss(str);
  std::string token;

  while (std::getline(ss, token, '\n')) {
    tokens.push_back(token);
  }
  return tokens;
}

bool Logger::init_logger(const std::string& logDir) {
  _logDirectory = logDir;
  try {
    bool dirExists = opendir(_logDirectory.c_str());
    if (!dirExists) {
      if (mkdir(_logDirectory.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
        log_fatal("Unable to create directory to write logs for nimbleSDK");
        // Unable to create directory to push logs.
        return false;
      }
    }
  } catch (...) {
    log_fatal("Unable to check for directory to write logs for nimbleSDK with exception");
    return false;
  }

  _writeFile = _logDirectory + "/latest.txt";
  _lastLogTime = std::chrono::high_resolution_clock::now() -
                 std::chrono::seconds(
                     2 * _timer_interval);  // setting time to be twice the timer interval
                                            // previous pending logs get uploaded at the first run
  _writeFilePtr = fopen(_writeFile.c_str(), "a+");

  if (_writeFilePtr == NULL) {
    log_fatal("Unable to create file to write logs for nimbleSDK");
    // Unable to open a new file to write logs
    return false;
  }
  _log_config._secretKey = _log_config._defaultSecretKey;
  return true;
}

void Logger::update_device_details(const std::string& deviceId, const std::string& clientId,
                                   bool debug) {
  _log_config._deviceId = deviceId;
  _log_config._clientId = clientId;
  _isDebug = debug;
}

void Logger::update_log_config(const std::string& secretKey, int time_interval,
                               const std::string& logUrl) {
  if (!secretKey.empty()) {
    _log_config._secretKey = secretKey;
  }
  _timer_interval = std::chrono::seconds(time_interval);
  _log_config._host = logUrl;
}

bool Logger::send_logs(const std::vector<std::string>& processed_logs,
                       const std::string& logfilePath) {
  if (processed_logs.empty()) {
    try {
      int didRemove = remove(logfilePath.c_str());
      if (didRemove) {
        LOG_TO_ERROR("%s could not be removed from the system. Failed with error %d",
                     logfilePath.c_str(), didRemove);
      }
    } catch (...) {
      LOG_TO_ERROR("%s Logfile could not be removed from the device.", logfilePath.c_str());
    }
    // Success since no logs need to be uploaded
    return true;
  }
  std::string log_string;
  json bodyarray = json::array({});

  json header = {
      {"Content-Type", "application/json"},
      {"DD-API-KEY", _log_config._secretKey},
      {"Accept", "application/json"},
  };
  for (auto log : processed_logs) {
    auto location = log.find(":::");
    std::string logType = log.substr(0, location);
    bodyarray.push_back({{"deviceID", _log_config._deviceId},
                         {"message", log},
                         // log[0] gives the type of Log Message sent to DataDog
                         {"logType", logType},
                         {"service", _log_config._service},
                         {"clientId", _log_config._clientId},
                         {"ddsource", _log_config._source},
                         {"ddtags", _log_config._tags}});
  };

  bool didSend = ServerAPI::upload_logs(LogRequestBody(header, bodyarray, _log_config._host));

  if (didSend) {
    try {
      int didRemove = remove(logfilePath.c_str());
      if (didRemove) {
        LOG_TO_ERROR("%s could not be removed from the system. Failed with error %d",
                     logfilePath.c_str(), didRemove);
      }
    } catch (...) {
      LOG_TO_ERROR("%s Logfile could not be removed from the device.", logfilePath.c_str());
    }
  }
  return didSend;
}

void Logger::send_pending_logs(bool initComplete) {
  auto time_now = std::chrono::high_resolution_clock::now();
  auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(time_now - _lastLogTime);
  if (elapsedTime > _timer_interval) {
    std::vector<std::string> logs;
    std::string logfilePath = getNewFileToSendLog(_logDirectory);
    // Case when there is no log file apart from latest.txt in the logs directory
    if (logfilePath == "") {
      if (_writeFilePtr == NULL) return;
      std::unique_lock<std::mutex> locker(_logMutex);
      if (!initComplete || ftell(_writeFilePtr) > MIN_LOG_FILE_SIZE_BYTES) {
        std::string newFileName = _logDirectory + "/log" + getStringDate() + ".txt";
        try {
          fclose(_writeFilePtr);
          int renameSuccess = rename(_writeFile.c_str(), newFileName.c_str());
          _writeFilePtr = fopen(_writeFile.c_str(), "a+");
          locker.unlock();
          logfilePath = newFileName;
        } catch (...) {
          _writeFilePtr = NULL;
          locker.unlock();
          log_error("Unable to rename log file");
          return;
        }
        std::ifstream logfile{logfilePath};
        std::string line;
        if (logfile) {
          while (getline(logfile, line)) {
            logs.push_back(line);
          }
        }
        logfile.close();
      } else {
        return;
      }
    } else {
      std::ifstream logfile{logfilePath};
      std::string line;
      if (logfile) {
        while (getline(logfile, line)) {
          logs.push_back(line);
        }
      }
      logfile.close();
    }

    send_logs(logs, logfilePath);
    _lastLogTime = std::chrono::high_resolution_clock::now();
  }
}
