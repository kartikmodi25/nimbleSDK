/* This is an Interface script written in C++
for sending, caching and loading files from the devices. */

#include "NativeInterface.h"

#include <fstream>

#include "client.h"
#include "util.h"
using namespace std;

namespace NativeInterface {
std::string HOMEDIR;

const std::shared_ptr<NetworkResponse> send_request(const string& body, const string& header,
                                                    const string& url, const string& method) {
  return std::make_shared<NetworkResponse>(
      ::send_request(body.c_str(), header.c_str(), url.c_str(), method.c_str()));
}

bool get_file_from_device(const std::string& fileName, string& result) {
  std::ifstream inFile(HOMEDIR + fileName, ios::binary);
  if (inFile.fail()) return false;
  result = string((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
  return true;
}

bool save_file_on_device(const std::string& content, const std::string& fileName, bool overWrite) {
  if (overWrite) {
    ofstream outFile(HOMEDIR + fileName, ios::out | ios::binary);
    outFile.write(content.c_str(), content.size());
  } else {
    ofstream outFile(HOMEDIR + fileName, ios::out | ios::binary | ios::app);
    outFile.write(content.c_str(), content.size());
  }
  return true;
}

char* get_metrics() { return ::get_metrics(SYSTEMMETRICS); }

}  // namespace NativeInterface
