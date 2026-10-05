#pragma once

#include <filesystem>

#include "ed/save.hpp"

namespace ed::server {

class SaveBackend {
public:
  explicit SaveBackend(std::filesystem::path root);
  bool save(const std::string& accountId, const ed::SaveSnapshot& snapshot) const;
  bool load(const std::string& accountId, ed::SaveSnapshot& snapshot) const;

private:
  std::filesystem::path root_;
};

} // namespace ed::server
