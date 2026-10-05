#include "postgres_backend.hpp"

#include <libpq-fe.h>
#include <openssl/rand.h>

#include <sstream>
#include <fstream>

namespace ed::server {
namespace {

std::string token() {
  unsigned char bytes[32];
  if (RAND_bytes(bytes, sizeof(bytes)) != 1) return {};
  static constexpr char digits[] = "0123456789abcdef";
  std::string result;
  for (const auto byte : bytes) {
    result.push_back(digits[byte >> 4]);
    result.push_back(digits[byte & 15]);
  }
  return result;
}

std::string hex(const std::string& value) {
  static constexpr char digits[] = "0123456789abcdef";
  std::string result;
  for (const unsigned char byte : value) {
    result.push_back(digits[byte >> 4]);
    result.push_back(digits[byte & 15]);
  }
  return result;
}

} // namespace

struct PostgresBackend::Impl {
  PGconn* connection = nullptr;
};

PostgresBackend::PostgresBackend(const std::string& connectionString)
    : impl_(std::make_unique<Impl>()) {
  impl_->connection = PQconnectdb(connectionString.c_str());
}

PostgresBackend::~PostgresBackend() {
  if (impl_->connection != nullptr) PQfinish(impl_->connection);
}

bool PostgresBackend::healthy() const {
  if (impl_->connection == nullptr || PQstatus(impl_->connection) != CONNECTION_OK) return false;
  PGresult* result = PQexec(impl_->connection, "SELECT 1");
  const bool ok = PQresultStatus(result) == PGRES_TUPLES_OK;
  PQclear(result);
  return ok;
}

bool PostgresBackend::migrate(const std::string& path) {
  std::ifstream input(path);
  if (!input) return false;
  std::ostringstream sql;
  sql << input.rdbuf();
  PGresult* result = PQexec(impl_->connection, sql.str().c_str());
  const bool ok = PQresultStatus(result) == PGRES_COMMAND_OK;
  PQclear(result);
  return ok;
}

AuthResult PostgresBackend::registerUser(const std::string& username, const std::string& password) {
  const char* values[] = {username.c_str(), password.c_str()};
  PGresult* result = PQexecParams(
      impl_->connection,
      "INSERT INTO accounts(username,password_hash) VALUES($1,crypt($2,gen_salt('bf',12))) "
      "RETURNING id",
      2, nullptr, values, nullptr, nullptr, 0);
  if (PQresultStatus(result) != PGRES_TUPLES_OK) {
    const std::string error = PQresultErrorField(result, PG_DIAG_SQLSTATE) == std::string("23505")
                                  ? "username already exists"
                                  : "account creation failed";
    PQclear(result);
    return {false, error == "username already exists" ? 409 : 500, {}, {}, error};
  }
  PQclear(result);
  return loginUser(username, password);
}

AuthResult PostgresBackend::loginUser(const std::string& username, const std::string& password) {
  const char* values[] = {username.c_str(), password.c_str()};
  PGresult* result = PQexecParams(
      impl_->connection,
      "SELECT id FROM accounts WHERE username=$1 AND password_hash=crypt($2,password_hash) "
      "AND status='active'",
      2, nullptr, values, nullptr, nullptr, 0);
  if (PQresultStatus(result) != PGRES_TUPLES_OK || PQntuples(result) != 1) {
    PQclear(result);
    return {false, 401, {}, {}, "invalid username or password"};
  }
  PQclear(result);
  const auto rawToken = token();
  const char* tokenValues[] = {username.c_str(), rawToken.c_str()};
  result = PQexecParams(
      impl_->connection,
      "INSERT INTO access_tokens(token_hash,account_id,expires_at) "
      "SELECT digest($2,'sha256'),id,now()+interval '15 minutes' FROM accounts WHERE username=$1 "
      "RETURNING token_hash",
      2, nullptr, tokenValues, nullptr, nullptr, 0);
  const bool ok = PQresultStatus(result) == PGRES_TUPLES_OK;
  PQclear(result);
  return ok ? AuthResult{true, 200, username, rawToken, {}} :
              AuthResult{false, 500, {}, {}, "session creation failed"};
}

std::string PostgresBackend::accountForToken(const std::string& rawToken) const {
  const char* values[] = {rawToken.c_str()};
  PGresult* result = PQexecParams(
      impl_->connection,
      "SELECT a.username FROM accounts a JOIN access_tokens t ON t.account_id=a.id "
      "WHERE t.token_hash=digest($1,'sha256') AND t.expires_at>now() AND t.revoked_at IS NULL",
      1, nullptr, values, nullptr, nullptr, 0);
  std::string username;
  if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) == 1) username = PQgetvalue(result, 0, 0);
  PQclear(result);
  return username;
}

bool PostgresBackend::load(const std::string& username, ed::SaveSnapshot& snapshot) const {
  const char* values[] = {username.c_str()};
  PGresult* result = PQexecParams(
      impl_->connection,
      "SELECT encode(blob,'hex') FROM saves s JOIN accounts a ON a.id=s.account_id "
      "WHERE a.username=$1 ORDER BY revision DESC LIMIT 1",
      1, nullptr, values, nullptr, nullptr, 0);
  if (PQresultStatus(result) != PGRES_TUPLES_OK || PQntuples(result) != 1) {
    PQclear(result);
    return false;
  }
  const std::string encoded = PQgetvalue(result, 0, 0);
  PQclear(result);
  std::string text;
  for (std::size_t i = 0; i + 1 < encoded.size(); i += 2) {
    text.push_back(static_cast<char>(std::stoi(encoded.substr(i, 2), nullptr, 16)));
  }
  return ed::deserializeSaveSnapshot(text, snapshot);
}

bool PostgresBackend::save(const std::string& username, const ed::SaveSnapshot& snapshot) {
  const auto text = ed::serializeSaveSnapshot(snapshot);
  const auto encoded = hex(text);
  const char* values[] = {username.c_str(), encoded.c_str()};
  PGresult* result = PQexecParams(
      impl_->connection,
      "INSERT INTO saves(account_id,revision,schema_version,blob,sha256) "
      "SELECT a.id,COALESCE((SELECT max(revision)+1 FROM saves WHERE account_id=a.id),1),1,"
      "decode($2,'hex'),digest(decode($2,'hex'),'sha256') FROM accounts a WHERE a.username=$1",
      2, nullptr, values, nullptr, nullptr, 0);
  const bool ok = PQresultStatus(result) == PGRES_COMMAND_OK;
  PQclear(result);
  return ok;
}

} // namespace ed::server
