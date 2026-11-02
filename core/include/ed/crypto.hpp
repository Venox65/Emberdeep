#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ed::crypto {

// SHA-256 hash (32 bytes output)
std::vector<std::uint8_t> sha256(const std::uint8_t* data, std::size_t len);
std::vector<std::uint8_t> sha256(const std::string& data);

// HMAC-SHA256 (32 bytes output)
std::vector<std::uint8_t> hmacSha256(const std::uint8_t* key, std::size_t keyLen,
                                    const std::uint8_t* data, std::size_t dataLen);
std::vector<std::uint8_t> hmacSha256(const std::string& key, const std::string& data);

// Constant-time memory comparison to protect against timing attacks
bool constantTimeCompare(const std::uint8_t* a, const std::uint8_t* b, std::size_t len);
bool constantTimeCompare(const std::string& a, const std::string& b);

// ChaCha20 stream cipher (symmetric encryption & decryption)
void chacha20(const std::uint8_t key[32],
              const std::uint8_t nonce[12],
              std::uint32_t counter,
              const std::uint8_t* input,
              std::uint8_t* output,
              std::size_t len);

std::string chacha20Crypt(const std::string& key,
                          const std::string& nonce,
                          const std::string& data,
                          std::uint32_t counter = 1);

// Cryptographically secure pseudorandom bytes using hardware entropy
std::vector<std::uint8_t> randomBytes(std::size_t count);

} // namespace ed::crypto
