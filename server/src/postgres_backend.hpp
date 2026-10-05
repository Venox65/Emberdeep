#pragma once

#include <memory>
#include <string>

#include "ed/save.hpp"

namespace ed::server {

struct AuthResult {
  bool ok = false;
  int status = 500;
  std::string username;
  std::string token;
  std::string message;
};

class PostgresBackend {
public:
  explicit PostgresBackend(const std::string& connectionString);
  ~PostgresBackend();
  PostgresBackend(const PostgresBackend&) = delete;
  PostgresBackend& operator=(const PostgresBackend&) = delete;

  bool healthy() const;
  bool migrate(const std::string& path);
  AuthResult registerUser(const std::string& username, const std::string& password);
  AuthResult loginUser(const std::string& username, const std::string& password);
  std::string accountForToken(const std::string& token) const;
  bool load(const std::string& username, ed::SaveSnapshot& snapshot) const;
  bool save(const std::string& username, const ed::SaveSnapshot& snapshot);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace ed::server
