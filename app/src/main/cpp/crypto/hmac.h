#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <string>

namespace crypto {
    std::array<uint8_t, 32> hmac_sha256(const uint8_t* key, size_t key_len,
                                         const uint8_t* data, size_t data_len);

    std::array<uint8_t, 64> hmac_sha512(const uint8_t* key, size_t key_len,
                                         const uint8_t* data, size_t data_len);

    // PBKDF2-HMAC-SHA512 (RFC 2898)
    // password: the password string
    // salt: the salt bytes
    // iterations: iteration count (e.g., 2048 for BIP-39)
    // dklen: desired key length in bytes
    std::vector<uint8_t> pbkdf2_hmac_sha512(const std::string& password,
                                             const std::vector<uint8_t>& salt,
                                             int iterations,
                                             int dklen);
}
