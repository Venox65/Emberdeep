#include "ed/crypto.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <random>

namespace ed::crypto {

namespace {

inline std::uint32_t rotr(std::uint32_t x, std::uint32_t n) {
  return (x >> n) | (x << (32 - n));
}

inline std::uint32_t rotl32(std::uint32_t v, std::uint32_t n) {
  return (v << n) | (v >> (32 - n));
}

inline std::uint32_t ch(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
  return (x & y) ^ (~x & z);
}

inline std::uint32_t maj(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
  return (x & y) ^ (x & z) ^ (y & z);
}

inline std::uint32_t sigma0(std::uint32_t x) {
  return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

inline std::uint32_t sigma1(std::uint32_t x) {
  return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

inline std::uint32_t gamma0(std::uint32_t x) {
  return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

inline std::uint32_t gamma1(std::uint32_t x) {
  return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

const std::uint32_t K256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

inline void quarterRound(std::uint32_t& a, std::uint32_t& b, std::uint32_t& c, std::uint32_t& d) {
  a += b; d ^= a; d = rotl32(d, 16);
  c += d; b ^= c; b = rotl32(b, 12);
  a += b; d ^= a; d = rotl32(d, 8);
  c += d; b ^= c; b = rotl32(b, 7);
}

inline std::uint32_t loadLittleEndian32(const std::uint8_t* p) {
  return static_cast<std::uint32_t>(p[0]) |
        (static_cast<std::uint32_t>(p[1]) << 8) |
        (static_cast<std::uint32_t>(p[2]) << 16) |
        (static_cast<std::uint32_t>(p[3]) << 24);
}

inline void storeLittleEndian32(std::uint8_t* p, std::uint32_t v) {
  p[0] = static_cast<std::uint8_t>(v & 0xFF);
  p[1] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
  p[2] = static_cast<std::uint8_t>((v >> 16) & 0xFF);
  p[3] = static_cast<std::uint8_t>((v >> 24) & 0xFF);
}

} // namespace

std::vector<std::uint8_t> sha256(const std::uint8_t* data, std::size_t len) {
  std::uint32_t H[8] = {
      0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
      0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
  };

  std::vector<std::uint8_t> msg(data, data + len);
  const std::uint64_t totalBits = static_cast<std::uint64_t>(len) * 8ULL;
  msg.push_back(0x80);
  while ((msg.size() % 64) != 56) {
    msg.push_back(0x00);
  }
  for (int i = 7; i >= 0; --i) {
    msg.push_back(static_cast<std::uint8_t>((totalBits >> (i * 8)) & 0xFF));
  }

  for (std::size_t chunk = 0; chunk < msg.size(); chunk += 64) {
    std::uint32_t W[64];
    for (int t = 0; t < 16; ++t) {
      W[t] = (static_cast<std::uint32_t>(msg[chunk + t * 4]) << 24) |
             (static_cast<std::uint32_t>(msg[chunk + t * 4 + 1]) << 16) |
             (static_cast<std::uint32_t>(msg[chunk + t * 4 + 2]) << 8) |
             (static_cast<std::uint32_t>(msg[chunk + t * 4 + 3]));
    }
    for (int t = 16; t < 64; ++t) {
      W[t] = gamma1(W[t - 2]) + W[t - 7] + gamma0(W[t - 15]) + W[t - 16];
    }

    std::uint32_t a = H[0], b = H[1], c = H[2], d = H[3];
    std::uint32_t e = H[4], f = H[5], g = H[6], h = H[7];

    for (int t = 0; t < 64; ++t) {
      const std::uint32_t T1 = h + sigma1(e) + ch(e, f, g) + K256[t] + W[t];
      const std::uint32_t T2 = sigma0(a) + maj(a, b, c);
      h = g;
      g = f;
      f = e;
      e = d + T1;
      d = c;
      c = b;
      b = a;
      a = T1 + T2;
    }

    H[0] += a; H[1] += b; H[2] += c; H[3] += d;
    H[4] += e; H[5] += f; H[6] += g; H[7] += h;
  }

  std::vector<std::uint8_t> digest(32);
  for (int i = 0; i < 8; ++i) {
    digest[i * 4]     = static_cast<std::uint8_t>((H[i] >> 24) & 0xFF);
    digest[i * 4 + 1] = static_cast<std::uint8_t>((H[i] >> 16) & 0xFF);
    digest[i * 4 + 2] = static_cast<std::uint8_t>((H[i] >> 8) & 0xFF);
    digest[i * 4 + 3] = static_cast<std::uint8_t>(H[i] & 0xFF);
  }
  return digest;
}

std::vector<std::uint8_t> sha256(const std::string& data) {
  return sha256(reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
}

std::vector<std::uint8_t> hmacSha256(const std::uint8_t* key, std::size_t keyLen,
                                    const std::uint8_t* data, std::size_t dataLen) {
  std::uint8_t k0[64] = {0};
  if (keyLen > 64) {
    auto keyHash = sha256(key, keyLen);
    std::memcpy(k0, keyHash.data(), keyHash.size());
  } else {
    std::memcpy(k0, key, keyLen);
  }

  std::vector<std::uint8_t> innerPad(64 + dataLen);
  for (int i = 0; i < 64; ++i) innerPad[i] = k0[i] ^ 0x36;
  if (dataLen > 0) std::memcpy(innerPad.data() + 64, data, dataLen);
  auto innerHash = sha256(innerPad.data(), innerPad.size());

  std::vector<std::uint8_t> outerPad(64 + 32);
  for (int i = 0; i < 64; ++i) outerPad[i] = k0[i] ^ 0x5c;
  std::memcpy(outerPad.data() + 64, innerHash.data(), 32);
  return sha256(outerPad.data(), outerPad.size());
}

std::vector<std::uint8_t> hmacSha256(const std::string& key, const std::string& data) {
  return hmacSha256(reinterpret_cast<const std::uint8_t*>(key.data()), key.size(),
                    reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
}

bool constantTimeCompare(const std::uint8_t* a, const std::uint8_t* b, std::size_t len) {
  std::uint8_t diff = 0;
  for (std::size_t i = 0; i < len; ++i) {
    diff |= (a[i] ^ b[i]);
  }
  return diff == 0;
}

bool constantTimeCompare(const std::string& a, const std::string& b) {
  if (a.size() != b.size()) return false;
  return constantTimeCompare(reinterpret_cast<const std::uint8_t*>(a.data()),
                             reinterpret_cast<const std::uint8_t*>(b.data()),
                             a.size());
}

void chacha20(const std::uint8_t key[32],
              const std::uint8_t nonce[12],
              std::uint32_t counter,
              const std::uint8_t* input,
              std::uint8_t* output,
              std::size_t len) {
  std::uint32_t state[16];
  // Constants: "expand 32-byte k"
  state[0] = 0x61707865;
  state[1] = 0x3320646e;
  state[2] = 0x79622d32;
  state[3] = 0x6b206574;

  for (int i = 0; i < 8; ++i) {
    state[4 + i] = loadLittleEndian32(key + i * 4);
  }
  state[12] = counter;
  state[13] = loadLittleEndian32(nonce + 0);
  state[14] = loadLittleEndian32(nonce + 4);
  state[15] = loadLittleEndian32(nonce + 8);

  std::size_t offset = 0;
  while (offset < len) {
    std::uint32_t working[16];
    for (int i = 0; i < 16; ++i) working[i] = state[i];

    for (int i = 0; i < 10; ++i) {
      quarterRound(working[0], working[4], working[8],  working[12]);
      quarterRound(working[1], working[5], working[9],  working[13]);
      quarterRound(working[2], working[6], working[10], working[14]);
      quarterRound(working[3], working[7], working[11], working[15]);

      quarterRound(working[0], working[5], working[10], working[15]);
      quarterRound(working[1], working[6], working[11], working[12]);
      quarterRound(working[2], working[7], working[8],  working[13]);
      quarterRound(working[3], working[4], working[9],  working[14]);
    }

    std::uint8_t block[64];
    for (int i = 0; i < 16; ++i) {
      storeLittleEndian32(block + i * 4, working[i] + state[i]);
    }

    const std::size_t take = std::min<std::size_t>(64, len - offset);
    for (std::size_t i = 0; i < take; ++i) {
      output[offset + i] = input[offset + i] ^ block[i];
    }
    offset += take;
    state[12]++;
  }
}

std::string chacha20Crypt(const std::string& key,
                          const std::string& nonce,
                          const std::string& data,
                          std::uint32_t counter) {
  if (key.size() != 32 || nonce.size() != 12) {
    return "";
  }
  std::string output(data.size(), '\0');
  chacha20(reinterpret_cast<const std::uint8_t*>(key.data()),
           reinterpret_cast<const std::uint8_t*>(nonce.data()),
           counter,
           reinterpret_cast<const std::uint8_t*>(data.data()),
           reinterpret_cast<std::uint8_t*>(output.data()),
           data.size());
  return output;
}

std::vector<std::uint8_t> randomBytes(std::size_t count) {
  std::vector<std::uint8_t> bytes(count);
  std::random_device rd;
  for (std::size_t i = 0; i < count; ++i) {
    bytes[i] = static_cast<std::uint8_t>(rd() & 0xFF);
  }
  return bytes;
}

} // namespace ed::crypto
