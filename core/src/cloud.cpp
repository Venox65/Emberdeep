#include "ed/cloud.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <sstream>

namespace ed {
namespace {

std::string hexEncode(const std::string& input) {
  static constexpr char digits[] = "0123456789abcdef";
  std::string result;
  result.reserve(input.size() * 2);
  for (const unsigned char byte : input) {
    result.push_back(digits[byte >> 4]);
    result.push_back(digits[byte & 15]);
  }
  return result;
}

std::string hexDecode(const std::string& input) {
  if (input.size() % 2 != 0) return {};
  std::string result;
  result.reserve(input.size() / 2);
  for (std::size_t i = 0; i < input.size(); i += 2) {
    const auto high = input[i];
    const auto low = input[i + 1];
    const auto value = [](char c) -> int {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'a' && c <= 'f') return c - 'a' + 10;
      if (c >= 'A' && c <= 'F') return c - 'A' + 10;
      return -1;
    };
    const int a = value(high);
    const int b = value(low);
    if (a < 0 || b < 0) return {};
    result.push_back(static_cast<char>((a << 4) | b));
  }
  return result;
}

CloudResult request(const std::string& host, int port, const std::string& method,
                    const std::string& path, const std::string& body,
                    const std::string& token) {
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo* result = nullptr;
  const auto portText = std::to_string(port);
  if (getaddrinfo(host.c_str(), portText.c_str(), &hints, &result) != 0) {
    return {false, 0, {}, "Unable to resolve backend host."};
  }
  int socketFd = -1;
  for (auto* address = result; address != nullptr; address = address->ai_next) {
    socketFd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (socketFd >= 0 && connect(socketFd, address->ai_addr, address->ai_addrlen) == 0) break;
    if (socketFd >= 0) close(socketFd);
    socketFd = -1;
  }
  freeaddrinfo(result);
  if (socketFd < 0) return {false, 0, {}, "Unable to connect to backend."};

  std::ostringstream message;
  message << method << " " << path << " HTTP/1.1\r\n"
          << "Host: " << host << "\r\n"
          << "Connection: close\r\n"
          << "Content-Type: application/json\r\n"
          << "Content-Length: " << body.size() << "\r\n";
  if (!token.empty()) message << "Authorization: Bearer " << token << "\r\n";
  message << "\r\n" << body;
  const auto wire = message.str();
  if (send(socketFd, wire.data(), wire.size(), 0) < 0) {
    close(socketFd);
    return {false, 0, {}, "Unable to send backend request."};
  }
  std::string response;
  char buffer[4096];
  ssize_t received = 0;
  while ((received = recv(socketFd, buffer, sizeof(buffer), 0)) > 0) {
    response.append(buffer, static_cast<std::size_t>(received));
  }
  close(socketFd);
  const auto headerEnd = response.find("\r\n\r\n");
  if (headerEnd == std::string::npos) return {false, 0, {}, "Malformed backend response."};
  const auto statusEnd = response.find(' ');
  if (statusEnd == std::string::npos) return {false, 0, {}, "Malformed backend status."};
  const int status = std::stoi(response.substr(statusEnd + 1, 3));
  const auto responseBody = response.substr(headerEnd + 4);
  return {status >= 200 && status < 300, status, responseBody,
          status >= 200 && status < 300 ? std::string{} : responseBody};
}

std::string field(const std::string& body, const std::string& key) {
  const auto marker = "\"" + key + "\":\"";
  const auto start = body.find(marker);
  if (start == std::string::npos) return {};
  const auto valueStart = start + marker.size();
  const auto end = body.find('"', valueStart);
  return end == std::string::npos ? std::string{} : body.substr(valueStart, end - valueStart);
}

} // namespace

bool readAccountProfile(const std::filesystem::path& path, AccountProfile& profile) {
  std::ifstream input(path);
  if (!input) return false;
  std::string line;
  while (std::getline(input, line)) {
    const auto split = line.find('=');
    if (split == std::string::npos) continue;
    const auto key = line.substr(0, split);
    const auto value = line.substr(split + 1);
    if (key == "mode") profile.mode = value;
    if (key == "username") profile.username = value;
    if (key == "token") profile.token = value;
  }
  return true;
}

bool writeAccountProfile(const std::filesystem::path& path, const AccountProfile& profile) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream output(path, std::ios::trunc);
  if (!output) return false;
  output << "mode=" << profile.mode << "\nusername=" << profile.username
         << "\ntoken=" << profile.token << "\n";
  return static_cast<bool>(output);
}

CloudResult cloudRegister(const std::string& host, int port,
                          const std::string& username, const std::string& password) {
  return request(host, port, "POST", "/v1/auth/register",
                 "{\"username\":\"" + username + "\",\"password\":\"" + password + "\"}", {});
}

CloudResult cloudLogin(const std::string& host, int port,
                       const std::string& username, const std::string& password) {
  return request(host, port, "POST", "/v1/auth/login",
                 "{\"username\":\"" + username + "\",\"password\":\"" + password + "\"}", {});
}

CloudResult cloudLoad(const std::string& host, int port, const std::string& token) {
  return request(host, port, "GET", "/v1/saves/latest", {}, token);
}

CloudResult cloudSave(const std::string& host, int port, const std::string& token,
                      const SaveSnapshot& snapshot) {
  return request(host, port, "PUT", "/v1/saves",
                 "{\"save\":\"" + hexEncode(serializeSaveSnapshot(snapshot)) + "\"}", token);
}

bool decodeCloudSnapshot(const std::string& body, SaveSnapshot& snapshot) {
  const auto encoded = field(body, "save");
  if (encoded.empty()) return false;
  const auto decoded = hexDecode(encoded);
  return !decoded.empty() && deserializeSaveSnapshot(decoded, snapshot);
}

std::string cloudResponseField(const CloudResult& result, const std::string& key) {
  return field(result.body, key);
}

} // namespace ed
