#include "save_backend.hpp"
#ifdef EMBERDEEP_ENABLE_POSTGRES
#include "postgres_backend.hpp"
#endif

#include <openssl/sha.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct User {
  std::string username;
  std::string salt;
  std::string passwordHash;
  std::string token;
};

std::string digest(const std::string& value) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(value.data()), value.size(), hash);
  std::ostringstream output;
  for (const auto byte : hash) output << std::hex << std::setw(2) << std::setfill('0')
                                      << static_cast<int>(byte);
  return output.str();
}

std::string jsonField(const std::string& body, const std::string& key) {
  const auto marker = "\"" + key + "\":\"";
  const auto start = body.find(marker);
  if (start == std::string::npos) return {};
  const auto valueStart = start + marker.size();
  const auto end = body.find('"', valueStart);
  return end == std::string::npos ? std::string{} : body.substr(valueStart, end - valueStart);
}

std::string json(const std::string& key, const std::string& value) {
  return "{\"" + key + "\":\"" + value + "\"}";
}

class Api {
public:
  explicit Api(std::filesystem::path root) : root_(std::move(root)), saves_(root_ / "saves") {
    loadUsers();
#ifdef EMBERDEEP_ENABLE_POSTGRES
    if (const char* databaseUrl = std::getenv("DATABASE_URL"); databaseUrl != nullptr) {
      postgres_ = std::make_unique<ed::server::PostgresBackend>(databaseUrl);
      const char* migrations = std::getenv("EMBERDEEP_MIGRATIONS");
      if (postgres_ != nullptr && migrations != nullptr) {
        postgresReady_ = true;
        std::vector<std::filesystem::path> migrationFiles;
        for (const auto& entry : std::filesystem::directory_iterator(migrations)) {
          if (!entry.is_regular_file() || entry.path().extension() != ".sql") continue;
          migrationFiles.push_back(entry.path());
        }
        std::sort(migrationFiles.begin(), migrationFiles.end());
        for (const auto& migration : migrationFiles) {
          postgresReady_ = postgres_->migrate(migration.string()) && postgresReady_;
        }
      }
    }
#endif
  }

  std::string handle(const std::string& method, const std::string& path,
                     const std::string& headers, const std::string& body, int& status) {
    if (method == "GET" && (path == "/health" || path == "/healthz")) {
      return response(200, "{\"status\":\"ok\"}", status);
    }
    if (method == "GET" && path == "/readyz") {
      bool ready = true;
#ifdef EMBERDEEP_ENABLE_POSTGRES
      ready = postgres_ == nullptr || (postgresReady_ && postgres_->healthy());
#endif
      return response(ready ? 200 : 503, ready ? "{\"status\":\"ready\"}" :
                                                    "{\"status\":\"not_ready\"}", status);
    }
    if (method == "POST" && path == "/v1/auth/register") {
      return registerUser(body, status);
    }
    if (method == "POST" && path == "/v1/auth/login") return loginUser(body, status);
    const auto token = header(headers, "Authorization");
    const auto account = accountForToken(token);
    if (account.empty()) return response(401, json("error", "authentication required"), status);
    if (method == "GET" && path == "/v1/saves/latest") {
      ed::SaveSnapshot snapshot;
      const bool loaded =
#ifdef EMBERDEEP_ENABLE_POSTGRES
          postgres_ != nullptr ? postgres_->load(account, snapshot) :
#endif
          saves_.load(account, snapshot);
      if (!loaded) {
        return response(404, json("error", "no cloud save"), status);
      }
      return response(200, json("save", hexEncode(ed::serializeSaveSnapshot(snapshot))), status);
    }
    if (method == "PUT" && path == "/v1/saves") {
      const auto encoded = jsonField(body, "save");
      const auto decoded = hexDecode(encoded);
      ed::SaveSnapshot snapshot;
      bool saved = false;
      if (!decoded.empty() && ed::deserializeSaveSnapshot(decoded, snapshot)) {
#ifdef EMBERDEEP_ENABLE_POSTGRES
        saved = postgres_ != nullptr ? postgres_->save(account, snapshot) : saves_.save(account, snapshot);
#else
        saved = saves_.save(account, snapshot);
#endif
      }
      if (!saved) {
        return response(400, json("error", "invalid save payload"), status);
      }
      return response(200, json("status", "saved"), status);
    }
    return response(404, json("error", "not found"), status);
  }

private:
  std::filesystem::path root_;
  ed::server::SaveBackend saves_;
  std::map<std::string, User> users_;
  std::mutex mutex_;
#ifdef EMBERDEEP_ENABLE_POSTGRES
  std::unique_ptr<ed::server::PostgresBackend> postgres_;
  bool postgresReady_ = false;
#endif

  static std::string hexEncode(const std::string& input) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string result;
    for (const unsigned char byte : input) {
      result.push_back(digits[byte >> 4]);
      result.push_back(digits[byte & 15]);
    }
    return result;
  }

  static std::string hexDecode(const std::string& input) {
    if (input.size() % 2 != 0) return {};
    std::string result;
    for (std::size_t i = 0; i < input.size(); i += 2) {
      const auto value = [](char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
      };
      const int high = value(input[i]);
      const int low = value(input[i + 1]);
      if (high < 0 || low < 0) return {};
      result.push_back(static_cast<char>((high << 4) | low));
    }
    return result;
  }

  static std::string header(const std::string& headers, const std::string& name) {
    const auto start = headers.find(name + ": ");
    if (start == std::string::npos) return {};
    const auto valueStart = start + name.size() + 2;
    const auto end = headers.find("\r\n", valueStart);
    return headers.substr(valueStart, end == std::string::npos ? std::string::npos : end - valueStart);
  }

  static std::string response(int code, const std::string& body, int& status) {
    status = code;
    return body;
  }

  std::string registerUser(const std::string& body, int& status) {
    const auto username = jsonField(body, "username");
    const auto password = jsonField(body, "password");
    if (username.size() < 3 || password.size() < 8 ||
        username.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") !=
            std::string::npos) {
      return response(400, json("error", "username must be 3+ alphanumeric characters and password 8+ characters"),
                      status);
    }
 #ifdef EMBERDEEP_ENABLE_POSTGRES
    if (postgres_ != nullptr) {
      const auto result = postgres_->registerUser(username, password);
      return response(result.status, result.ok
          ? "{\"token\":\"" + result.token + "\",\"username\":\"" + result.username + "\"}"
          : json("error", result.message), status);
    }
 #endif
    std::lock_guard lock(mutex_);
    if (users_.contains(username)) return response(409, json("error", "username already exists"), status);
    User user{username, digest(username + "-emberdeep-salt"), {}, {}};
    user.passwordHash = digest(user.salt + password);
    user.token = digest(username + user.passwordHash + "session");
    users_[username] = user;
    saveUsers();
    return response(201, "{\"token\":\"" + user.token + "\",\"username\":\"" + username + "\"}", status);
  }

  std::string loginUser(const std::string& body, int& status) {
    const auto username = jsonField(body, "username");
    const auto password = jsonField(body, "password");
 #ifdef EMBERDEEP_ENABLE_POSTGRES
    if (postgres_ != nullptr) {
      const auto result = postgres_->loginUser(username, password);
      return response(result.status, result.ok
          ? "{\"token\":\"" + result.token + "\",\"username\":\"" + result.username + "\"}"
          : json("error", result.message), status);
    }
 #endif
    std::lock_guard lock(mutex_);
    const auto it = users_.find(username);
    if (it == users_.end() || it->second.passwordHash != digest(it->second.salt + password)) {
      return response(401, json("error", "invalid username or password"), status);
    }
    return response(200, "{\"token\":\"" + it->second.token + "\",\"username\":\"" + username + "\"}", status);
  }

  std::string accountForToken(const std::string& authorization) const {
    const auto prefix = std::string("Bearer ");
    if (authorization.rfind(prefix, 0) != 0) return {};
#ifdef EMBERDEEP_ENABLE_POSTGRES
    if (postgres_ != nullptr) return postgres_->accountForToken(authorization.substr(prefix.size()));
#endif
    for (const auto& [username, user] : users_) {
      if (user.token == authorization.substr(prefix.size())) return username;
    }
    return {};
  }

  void loadUsers() {
    std::ifstream input(root_ / "users.db");
    std::string line;
    while (std::getline(input, line)) {
      std::istringstream row(line);
      User user;
      std::getline(row, user.username, '|');
      std::getline(row, user.salt, '|');
      std::getline(row, user.passwordHash, '|');
      std::getline(row, user.token, '|');
      if (!user.username.empty()) users_[user.username] = user;
    }
  }

  void saveUsers() const {
    std::filesystem::create_directories(root_);
    std::ofstream output(root_ / "users.db", std::ios::trunc);
    for (const auto& [_, user] : users_) {
      output << user.username << "|" << user.salt << "|" << user.passwordHash << "|" << user.token << "\n";
    }
  }
};

void serve(Api& api, int client) {
  std::string request;
  char buffer[4096];
  ssize_t received = 0;
  while ((received = recv(client, buffer, sizeof(buffer), 0)) > 0) {
    request.append(buffer, static_cast<std::size_t>(received));
    if (request.find("\r\n\r\n") != std::string::npos) break;
  }
  const auto headerEnd = request.find("\r\n\r\n");
  if (headerEnd == std::string::npos) {
    close(client);
    return;
  }
  const auto requestLineEnd = request.find("\r\n");
  std::istringstream requestLine(request.substr(0, requestLineEnd));
  std::string method;
  std::string path;
  std::string version;
  requestLine >> method >> path >> version;
  const auto headers = request.substr(requestLineEnd + 2, headerEnd - requestLineEnd - 2);
  std::size_t contentLength = 0;
  const auto lengthMarker = headers.find("Content-Length: ");
  if (lengthMarker != std::string::npos) {
    contentLength = std::stoul(headers.substr(lengthMarker + 16));
  }
  std::string body = request.substr(headerEnd + 4);
  while (body.size() < contentLength &&
         (received = recv(client, buffer, sizeof(buffer), 0)) > 0) {
    body.append(buffer, static_cast<std::size_t>(received));
  }
  int status = 500;
  const auto responseBody = api.handle(method, path, headers, body, status);
  const std::string statusText = status == 200 ? "OK" : status == 201 ? "Created" :
      status == 400 ? "Bad Request" : status == 401 ? "Unauthorized" :
      status == 404 ? "Not Found" : status == 409 ? "Conflict" : "Error";
  std::ostringstream response;
  response << "HTTP/1.1 " << status << " " << statusText << "\r\n"
           << "Content-Type: application/json\r\nConnection: close\r\nContent-Length: "
           << responseBody.size() << "\r\n\r\n" << responseBody;
  const auto wire = response.str();
  send(client, wire.data(), wire.size(), 0);
  close(client);
}

} // namespace

int main(int argc, char** argv) {
  const auto env = [](const char* name, const char* fallback) {
    const char* value = std::getenv(name);
    return value == nullptr || *value == '\0' ? std::string(fallback) : std::string(value);
  };
  const int port = argc > 1 ? std::stoi(argv[1]) : std::stoi(env("EMBERDEEP_PORT", "8080"));
  const std::filesystem::path dataRoot =
      argc > 2 ? argv[2] : std::filesystem::path(env("EMBERDEEP_DATA_ROOT", "server-data"));
  const std::string host = env("EMBERDEEP_HOST", "0.0.0.0");
  Api api(dataRoot);
  const int server = socket(AF_INET, SOCK_STREAM, 0);
  int reuse = 1;
  setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  sockaddr_in address{};
  address.sin_family = AF_INET;
  if (host == "127.0.0.1" || host == "localhost") {
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
  } else {
    address.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  address.sin_port = htons(static_cast<uint16_t>(port));
  if (bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0 ||
      listen(server, 16) < 0) {
    std::cerr << "Unable to listen on port " << port << "\n";
    return 1;
  }
  std::cout << "Emberdeep API listening on port " << port
            << " using data directory " << dataRoot << "\n";
  while (true) {
    const int client = accept(server, nullptr, nullptr);
    if (client >= 0) serve(api, client);
  }
}
