#pragma once
#include "CommonClient.h"

CNetworkResponse send_request(const char* body, const char* headers, const char* url,
                              const char* method);

char* get_battery_status();

char* get_temperature_status();

char* get_cpu_status();

char* get_memory_status();

char* get_hardware_info();

char* save(const char* inputStream, const char* filePath, int overwrite);

int deleteData(const char* filePath);

char* retrieve(const char* filePath);

int check_if_exists(const char* filePath);

void log_debug(const char* message);

void log_info(const char* message);

void log_warn(const char* message);

void log_error(const char* message);

void log_fatal(const char* message);

void init_logger(const char* deviceId, const char* apiKey);

void send_pending_logs();

char* save_to_cache(const char* data, const char* filePath);

char* retrieve_from_cache(const char* filePath);

char* get_metrics(const char* metricType);