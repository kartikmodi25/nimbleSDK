#pragma once
#include "Logger.h"
#include "base64.h"
#include "crossPlatformUtil.h"
#include "json.h"

#define API_VERSION "/api/v1"

#define DEFAULT_AUTH_ERROR_RETRIES 3
#define DEFAULT_LOAD_MODEL_RETRIES 3
#define MAX_LOG_FILE_SIZE_IN_KB 500
#define MIN_LOG_FILE_SIZE_BYTES 1000
#define METRICS_TIMER_INTERVAL_IN_SECONDS 5
#define METRICS_COLLECTION_TIME_INTERVAL_IN_SECONDS 20
#define DEFAULT_THREAD_SLEEP_TIME_MICROSECONDS 1000000
#define MAX_BYTES_IN_KB 1024
#define HTTPSERVICE "/mds"
#define FEATURESERVICE "/nimblesync"

#define INFERENCEMETADATAFILENAME "inferencePlanData.txt"
#define INFERENCEFILENAME "inferencePlan.txt"

#define FEATURESTOREMETADATAFILENAME "featureStore.txt"
#define FEATURESTOREFILENAME "featureVector.txt"

#define SYSTEMMETRICS "system-metrics"

#define DEFAULT_SQLITE_DB_NAME "nimbleDB"

#define DEFAULT_REGISTER_RETRIES 3

#define DEFAULT_FETCH_CLOUD_CONFIG_RETRIES 3
