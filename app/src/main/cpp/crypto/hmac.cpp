// HMAC-SHA256, HMAC-SHA512, PBKDF2-HMAC-SHA512
// RFC 2104 (HMAC), RFC 2898 (PBKDF2)
#include "hmac.h"
#include "sha256.h"
#include "sha512.h"
#include <cstring>

namespace crypto {

// HMAC-SHA256 (RFC 2104)
// Block size for SHA-256 is 64 bytes
std::array<uint8_t, 32> hmac_sha256(const uint8_t* key, size_t key_len,
                                     const uint8_t* data, size_t data_len) {
    constexpr size_t BLOCK = 64;
    uint8_t k[BLOCK] = {};

    // If key is longer than block size, hash it first
    if (key_len > BLOCK) {
        auto hk = sha256(key, key_len);
        std::memcpy(k, hk.data(), 32);
    } else {
        std::memcpy(k, key, key_len);
    }

    // Build i_key_pad and o_key_pad
    uint8_t i_key_pad[BLOCK], o_key_pad[BLOCK];
    for (size_t i = 0; i < BLOCK; ++i) {
        i_key_pad[i] = k[i] ^ 0x36;
        o_key_pad[i] = k[i] ^ 0x5c;
    }

    // inner = SHA256(i_key_pad || data)
    std::vector<uint8_t> inner_msg(BLOCK + data_len);
    std::memcpy(inner_msg.data(), i_key_pad, BLOCK);
    std::memcpy(inner_msg.data() + BLOCK, data, data_len);
    auto inner_hash = sha256(inner_msg.data(), inner_msg.size());

    // outer = SHA256(o_key_pad || inner)
    std::vector<uint8_t> outer_msg(BLOCK + 32);
    std::memcpy(outer_msg.data(), o_key_pad, BLOCK);
    std::memcpy(outer_msg.data() + BLOCK, inner_hash.data(), 32);
    return sha256(outer_msg.data(), outer_msg.size());
}

// HMAC-SHA512 (RFC 2104)
// Block size for SHA-512 is 128 bytes
std::array<uint8_t, 64> hmac_sha512(const uint8_t* key, size_t key_len,
                                     const uint8_t* data, size_t data_len) {
    constexpr size_t BLOCK = 128;
    uint8_t k[BLOCK] = {};

    // If key is longer than block size, hash it first
    if (key_len > BLOCK) {
        auto hk = sha512(key, key_len);
        std::memcpy(k, hk.data(), 64);
    } else {
        std::memcpy(k, key, key_len);
    }

    // Build i_key_pad and o_key_pad
    uint8_t i_key_pad[BLOCK], o_key_pad[BLOCK];
    for (size_t i = 0; i < BLOCK; ++i) {
        i_key_pad[i] = k[i] ^ 0x36;
        o_key_pad[i] = k[i] ^ 0x5c;
    }

    // inner = SHA512(i_key_pad || data)
    std::vector<uint8_t> inner_msg(BLOCK + data_len);
    std::memcpy(inner_msg.data(), i_key_pad, BLOCK);
    std::memcpy(inner_msg.data() + BLOCK, data, data_len);
    auto inner_hash = sha512(inner_msg.data(), inner_msg.size());

    // outer = SHA512(o_key_pad || inner)
    std::vector<uint8_t> outer_msg(BLOCK + 64);
    std::memcpy(outer_msg.data(), o_key_pad, BLOCK);
    std::memcpy(outer_msg.data() + BLOCK, inner_hash.data(), 64);
    return sha512(outer_msg.data(), outer_msg.size());
}

// PBKDF2-HMAC-SHA512 (RFC 2898 Section 5.2)
// DK = T1 || T2 || ... || Tdklen/hlen
// Ti = F(Password, Salt, c, i)
// F(Password, Salt, c, i) = U1 ^ U2 ^ ... ^ Uc
// U1 = PRF(Password, Salt || INT(i))
// Uj = PRF(Password, U{j-1})
std::vector<uint8_t> pbkdf2_hmac_sha512(const std::string& password,
                                         const std::vector<uint8_t>& salt,
                                         int iterations,
                                         int dklen) {
    constexpr int hlen = 64; // SHA-512 output is 64 bytes
    const uint8_t* pw = reinterpret_cast<const uint8_t*>(password.data());
    const size_t pw_len = password.size();

    std::vector<uint8_t> dk;
    dk.reserve(static_cast<size_t>(dklen));

    int block_count = (dklen + hlen - 1) / hlen;

    for (int block_idx = 1; block_idx <= block_count; ++block_idx) {
        // U1 = HMAC-SHA512(password, salt || INT(block_idx))
        // INT(i) is big-endian 4-byte integer
        std::vector<uint8_t> salt_with_block(salt.size() + 4);
        std::memcpy(salt_with_block.data(), salt.data(), salt.size());
        salt_with_block[salt.size() + 0] = (block_idx >> 24) & 0xff;
        salt_with_block[salt.size() + 1] = (block_idx >> 16) & 0xff;
        salt_with_block[salt.size() + 2] = (block_idx >>  8) & 0xff;
        salt_with_block[salt.size() + 3] = (block_idx      ) & 0xff;

        auto U = hmac_sha512(pw, pw_len,
                             salt_with_block.data(), salt_with_block.size());
        auto T = U; // T = U1 initially

        // U2..Uc: each Uj = HMAC-SHA512(password, U{j-1})
        for (int iter = 1; iter < iterations; ++iter) {
            U = hmac_sha512(pw, pw_len, U.data(), U.size());
            for (int b = 0; b < hlen; ++b)
                T[b] ^= U[b];
        }

        // Append T to dk (possibly truncated on last block)
        int bytes_to_add = std::min(hlen, dklen - (int)dk.size());
        dk.insert(dk.end(), T.begin(), T.begin() + bytes_to_add);
    }

    return dk;
}

} // namespace crypto
