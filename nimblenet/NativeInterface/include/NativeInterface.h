#pragma once
#include "NativeInterfaceStructs.h"
#include "util.h"

namespace NativeInterface {
extern std::string HOMEDIR;
const std::shared_ptr<NetworkResponse> send_request(const std::string& body,
                                                    const std::string& header,
                                                    const std::string& url,
                                                    const std::string& method);
bool get_file_from_device(const std::string& fileName, std::string& result);
bool save_file_on_device(const std::string& content, const std::string& fileName,
                         bool overWrite = true);
char* get_metrics();
}  // namespace NativeInterface
