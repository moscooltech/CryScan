// Complete SHA-256 implementation (FIPS 180-4)
// Test vector: sha256("abc") = ba7816bf 8f01cfea 414140de 5dae2ec7 3b338c18 6a33ce9c d7b06f19 9d04df00
//              sha256("")    = e3b0c442 98fc1c14 9afbf4c8 996fb924 27ae41e4 649b934c a495991b 7852b855
#include "sha256.h"
#include <cstring>

namespace crypto {

namespace {

// SHA-256 Constants (first 32 bits of fractional parts of cube roots of first 64 primes)
static constexpr uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// Initial hash values (first 32 bits of fractional parts of square roots of first 8 primes)
static constexpr uint32_t H0[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

inline uint32_t rotr32(uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

inline uint32_t Ch(uint32_t e, uint32_t f, uint32_t g)  { return (e & f) ^ (~e & g); }
inline uint32_t Maj(uint32_t a, uint32_t b, uint32_t c) { return (a & b) ^ (a & c) ^ (b & c); }
inline uint32_t Sigma0(uint32_t x) { return rotr32(x, 2) ^ rotr32(x, 13) ^ rotr32(x, 22); }
inline uint32_t Sigma1(uint32_t x) { return rotr32(x, 6) ^ rotr32(x, 11) ^ rotr32(x, 25); }
inline uint32_t sigma0(uint32_t x) { return rotr32(x, 7) ^ rotr32(x, 18) ^ (x >> 3); }
inline uint32_t sigma1(uint32_t x) { return rotr32(x, 17) ^ rotr32(x, 19) ^ (x >> 10); }

inline uint32_t be32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  | (uint32_t)p[3];
}

inline void store_be32(uint8_t* p, uint32_t v) {
    p[0] = (v >> 24) & 0xff;
    p[1] = (v >> 16) & 0xff;
    p[2] = (v >> 8)  & 0xff;
    p[3] = v & 0xff;
}

struct Sha256State {
    uint32_t h[8];

    Sha256State() {
        for (int i = 0; i < 8; ++i) h[i] = H0[i];
    }

    void processBlock(const uint8_t block[64]) {
        uint32_t W[64];
        for (int i = 0; i < 16; ++i)
            W[i] = be32(block + i * 4);
        for (int i = 16; i < 64; ++i)
            W[i] = sigma1(W[i-2]) + W[i-7] + sigma0(W[i-15]) + W[i-16];

        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];

        for (int i = 0; i < 64; ++i) {
            uint32_t T1 = hh + Sigma1(e) + Ch(e, f, g) + K[i] + W[i];
            uint32_t T2 = Sigma0(a) + Maj(a, b, c);
            hh = g; g = f; f = e; e = d + T1;
            d = c; c = b; b = a; a = T1 + T2;
        }

        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }

    std::array<uint8_t, 32> digest() const {
        std::array<uint8_t, 32> out;
        for (int i = 0; i < 8; ++i)
            store_be32(out.data() + i * 4, h[i]);
        return out;
    }
};

} // anonymous namespace

std::array<uint8_t, 32> sha256(const uint8_t* data, size_t len) {
    Sha256State state;

    // Process complete 64-byte blocks
    size_t offset = 0;
    while (offset + 64 <= len) {
        state.processBlock(data + offset);
        offset += 64;
    }

    // Build padded final block(s)
    // Padding: 1-bit (0x80), zeros, then 64-bit big-endian length in bits
    uint8_t block[128] = {};
    size_t remainder = len - offset;
    std::memcpy(block, data + offset, remainder);
    block[remainder] = 0x80;

    // The 8-byte length field (in bits, big-endian)
    uint64_t bitlen = (uint64_t)len * 8;

    if (remainder < 56) {
        // Length fits in first padding block
        for (int i = 0; i < 8; ++i)
            block[63 - i] = (uint8_t)(bitlen >> (8 * i));
        state.processBlock(block);
    } else {
        // Need two blocks
        for (int i = 0; i < 8; ++i)
            block[127 - i] = (uint8_t)(bitlen >> (8 * i));
        state.processBlock(block);
        state.processBlock(block + 64);
    }

    return state.digest();
}

std::array<uint8_t, 32> sha256(const std::vector<uint8_t>& data) {
    return sha256(data.data(), data.size());
}

std::array<uint8_t, 32> sha256d(const uint8_t* data, size_t len) {
    auto first = sha256(data, len);
    return sha256(first.data(), 32);
}

} // namespace crypto
