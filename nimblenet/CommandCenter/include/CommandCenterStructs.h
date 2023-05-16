#pragma once
#include "util.h"

struct CloudConfig {
  std::string logKey;
  std::string metricsKey;
  std::string logUrl;
  int logTimeInterval;
  int metricsTimeInterval;
  int metricsCollectionInterval;

  CloudConfig() {
    logKey = "";
    metricsKey = "";
    logTimeInterval = LOG_TIMER_INTERVAL_IN_SECONDS;
    metricsTimeInterval = METRICS_TIMER_INTERVAL_IN_SECONDS;
    metricsCollectionInterval = METRICS_COLLECTION_TIME_INTERVAL_IN_SECONDS;
    logUrl = DEFAULT_UPLOAD_LOGS_URL;
  }
};