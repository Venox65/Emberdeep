#include "save_backend.hpp"

namespace ed::server {

SaveBackend::SaveBackend(std::filesystem::path root) : root_(std::move(root)) {
}

bool SaveBackend::save(const std::string& accountId, const ed::SaveSnapshot& snapshot) const {
  return ed::writeSaveAtomic(root_ / (accountId + ".sav"), snapshot);
}

bool SaveBackend::load(const std::string& accountId, ed::SaveSnapshot& snapshot) const {
  return ed::readSave(root_ / (accountId + ".sav"), snapshot);
}

} // namespace ed::server
