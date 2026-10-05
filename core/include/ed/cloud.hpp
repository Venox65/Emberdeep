#pragma once

#include <filesystem>
#include <string>

#include "ed/save.hpp"

namespace ed {

struct CloudResult {
  bool ok = false;
  int status = 0;
  std::string body;
  std::string error;
};

struct AccountProfile {
  std::string mode = "guest";
  std::string username;
  std::string token;
};

bool readAccountProfile(const std::filesystem::path& path, AccountProfile& profile);
bool writeAccountProfile(const std::filesystem::path& path, const AccountProfile& profile);

CloudResult cloudRegister(const std::string& host, int port,
                          const std::string& username, const std::string& password);
CloudResult cloudLogin(const std::string& host, int port,
                       const std::string& username, const std::string& password);
CloudResult cloudLoad(const std::string& host, int port, const std::string& token);
CloudResult cloudSave(const std::string& host, int port, const std::string& token,
                      const SaveSnapshot& snapshot);
std::string cloudResponseField(const CloudResult& result, const std::string& key);
bool decodeCloudSnapshot(const std::string& body, SaveSnapshot& snapshot);

} // namespace ed
