#include "client.h"

#include <curl/curl.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

struct stringy {
  char *ptr;
  size_t len;
};

void init_string(struct stringy *s) {
  s->len = 0;
  s->ptr = (char *)malloc(s->len + 1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct stringy *s) {
  size_t new_len = s->len + size * nmemb;
  s->ptr = (char *)realloc(s->ptr, new_len + 1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr + s->len, ptr, size * nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;
  return size * nmemb;
}

CNetworkResponse send_request(const char *body, const char *headers_c, const char *url,
                              const char *method) {
  CURL *curl;
  CURLcode res;
  struct curl_slist *headers = NULL;
  // try catch since empty headers leads to json errors and we send empty headers on Register calls
  try {
    nlohmann::json jsonheaders = nlohmann::json::parse(headers_c);
    for (auto &header : jsonheaders.items()) {
      std::string hval = header.key() + ": " + std::string(jsonheaders[header.key()]);
      // hval = hval + header.value();
      // printf("Header is %s", hval.c_str());
      headers = curl_slist_append(headers, hval.c_str());
    }
  } catch (std::out_of_range &e) {
    std::cout << "out of range: " << e.what() << std::endl;
  } catch (std::invalid_argument &e) {
    std::cout << "invalid" << std::endl;
  } catch (std::exception &e) {
    std::cout << "Exception :" << e.what() << std::endl;
  }

  curl = curl_easy_init();
  std::string URL = url;
  // std::cout<<URL<<std::endl;
  CNetworkResponse response;
  response.statusCode = 900;
  response.body = "";
  response.bodyLength = 0;
  response.headers = "";
  if (!curl) abort();
  /* First set the URL that is about to receive our POST. This URL can
   *        just as well be a https:// URL if that is what should receive the
   *               data. */
  struct stringy s;
  init_string(&s);
  curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING,
                   "");  // This accepts all formats of encoding supported by curl
  if (strcmp(method, "GET")) curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writefunc);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
  /* Perform the request, res will get the return code */
  char buffer[CURL_ERROR_SIZE + 1] = {};
  auto retCode = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, buffer);
  res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    response.statusCode = 900;
    response.body = nullptr;
    response.headers = nullptr;
    response.bodyLength = 0;
    return response;
  }
  // printf("%s\n",s.ptr);
  long http_code = 0;
  res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  if (res != CURLE_OK) {
    response.statusCode = 900;
    response.body = nullptr;
    response.headers = nullptr;
    response.bodyLength = 0;
    return response;
  }

  response.statusCode = http_code;
  response.body = s.ptr;
  response.bodyLength = s.len;
  response.headers = (char *)malloc(10);  // TODO - Pass response headers from the curl output
  // printf("%s status_code=%ld\n",buffer,http_code);
  curl_easy_cleanup(curl);
  return response;
}

char *get_battery_status() {
  return "{\"status\":\"success\",\"error\":\"\",\"battery_level\":50,"
         "\"charging_status\":true}";
}

char *get_temperature_status() {
  return "{\"status\":\"success\",\"error\":\"\",\"cpu_temp\":20,\"gpu_"
         "temp\":"
         "20,\"battery_temp\":20}";
}

char *get_cpu_status() { return "{\"status\":\"success\",\"usage\":77.9, \"gpu_load\":10.9}"; }

char *get_memory_status() { return "{\"status\":\"success\", \"ram\": 5.5, \"storage\": 70.4}"; }

char *get_hardware_info() {
  return "{\"status\":\"success\", \"kernel_version\":\"kernel1\", "
         "\"model\":\"cpu model1\", \"cpu_arch\":\"cpu architecture\", "
         "\"cpu_num\":8, \"clock_speed\": \"8.5\", \"cache\":12, "
         "\"cores\":8, "
         "\"threads\":3, gpu_vendor:\"gpu vendor\", \"gpu_renderer\":\"gpu "
         "Renderer 1\", \"gpu_version\":\"GPU Version 1\"}";
}

char *save(const char *inputStream, const char *filePath, int overwrite) {
  FILE *fptr;

  if (overwrite) {
    fptr = fopen(filePath, "w");
  } else {
    fptr = fopen(filePath, "a+");
  }

  fprintf(fptr, "%s", inputStream);
  fclose(fptr);

  char *string;
  asprintf(&string, "{\"status\":true, \"error\":\"\", \"file_location\":\"%s\"}", filePath);

  return string;
}

int deleteData(const char *filePath) {
  remove(filePath);
  return 1;
}

char *retrieve(const char *filePath) {
  char *text;
  long numbytes;
  char *string;
  FILE *textfile = fopen(filePath, "r");
  if (textfile == NULL) {
    asprintf(&string, "{\"status\":false, \"error\":\"ERROR\", \"data\":\"\"}");
    return string;
  }

  fseek(textfile, 0L, SEEK_END);
  numbytes = ftell(textfile);
  fseek(textfile, 0L, SEEK_SET);

  text = (char *)malloc(numbytes + 1);
  fread(text, numbytes, 1, textfile);
  text[numbytes] = 0;

  if (text == NULL) {
    printf("NUll file\n");
    asprintf(&string, "{\"status\":false, \"error\":\"ERROR\", \"data\":\"\"}");
  } else {
    asprintf(&string, "{\"status\":true, \"error\":\"\", \"data\":%s}", text);
  }

  fclose(textfile);
  return string;
}

int check_if_exists(const char *filePath) {
  char *text;
  long numbytes;

  FILE *textfile = fopen(filePath, "r");
  if (textfile == NULL) return 0;
  return 1;
}

void log_debug(const char *message) { printf("DEBUG:%s\n", message); }

void log_info(const char *message) { printf("INFO:%s\n", message); }

void log_warn(const char *message) { printf("WARN:%s\n", message); }

void log_error(const char *message) { printf("ERROR:%s\n", message); }

void log_fatal(const char *message) { printf("FATAL:%s\n", message); }

char *save_to_cache(const char *data, const char *filePath) { return save(data, filePath, 1); }

char *retrieve_from_cache(const char *filePath) { return retrieve(filePath); }

char *get_metrics(const char *metricType) {
  char *m;
  asprintf(&m, "%s", "{}");
  return m;
}
