// Keccak-256 implementation (Ethereum uses Keccak-256, NOT NIST SHA3-256)
// The difference: Keccak uses domain suffix 0x01, NIST SHA3 uses 0x06
// Reference: Keccak reference implementation, NIST competition submission
// Test: keccak256("") = c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
#include "keccak256.h"
#include <cstring>

namespace crypto {

namespace {

// Keccak-f[1600] round constants
static constexpr uint64_t RC[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL,
    0x800000000000808AULL, 0x8000000080008000ULL,
    0x000000000000808BULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008AULL, 0x0000000000000088ULL,
    0x0000000080008009ULL, 0x000000008000000AULL,
    0x000000008000808BULL, 0x800000000000008BULL,
    0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800AULL, 0x800000008000000AULL,
    0x8000000080008081ULL, 0x8000000000008080ULL,
    0x0000000080000001ULL, 0x8000000080008008ULL
};

// Rotation offsets for rho step
static constexpr int RHO[24] = {
     1,  3,  6, 10, 15, 21, 28, 36, 45, 55,  2, 14,
    27, 41, 56,  8, 25, 43, 62, 18, 39, 61, 20, 44
};

// Pi step permutation indices (x,y) -> (y, 2x+3y mod 5)
static constexpr int PI[24] = {
    10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4,
    15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6, 1
};

inline uint64_t rotl64(uint64_t x, int n) {
    return (x << n) | (x >> (64 - n));
}

// Keccak-f[1600] permutation
void keccakF1600(uint64_t state[25]) {
    for (int round = 0; round < 24; ++round) {
        // Theta step
        uint64_t C[5], D[5];
        for (int x = 0; x < 5; ++x)
            C[x] = state[x] ^ state[x+5] ^ state[x+10] ^ state[x+15] ^ state[x+20];
        for (int x = 0; x < 5; ++x)
            D[x] = C[(x+4)%5] ^ rotl64(C[(x+1)%5], 1);
        for (int i = 0; i < 25; ++i)
            state[i] ^= D[i % 5];

        // Rho and Pi steps combined
        uint64_t last = state[1];
        for (int i = 0; i < 24; ++i) {
            int j = PI[i];
            uint64_t temp = state[j];
            state[j] = rotl64(last, RHO[i]);
            last = temp;
        }

        // Chi step
        for (int y = 0; y < 25; y += 5) {
            uint64_t t[5];
            for (int x = 0; x < 5; ++x) t[x] = state[y + x];
            for (int x = 0; x < 5; ++x)
                state[y + x] = t[x] ^ (~t[(x+1)%5] & t[(x+2)%5]);
        }

        // Iota step
        state[0] ^= RC[round];
    }
}

// Keccak sponge function
// rate: 1088 bits = 136 bytes for keccak-256 (capacity = 512 bits)
// delimiter: 0x01 for Keccak (not NIST SHA3 which uses 0x06)
std::array<uint8_t, 32> keccakHash(const uint8_t* data, size_t len,
                                    size_t rate, uint8_t delimiter) {
    uint64_t state[25] = {};
    const size_t rate_bytes = rate / 8;

    // Absorb phase
    size_t offset = 0;
    while (offset + rate_bytes <= len) {
        for (size_t i = 0; i < rate_bytes / 8; ++i) {
            uint64_t lane = 0;
            for (int b = 0; b < 8; ++b)
                lane |= (uint64_t)data[offset + i*8 + b] << (8*b);
            state[i] ^= lane;
        }
        keccakF1600(state);
        offset += rate_bytes;
    }

    // Padding block
    uint8_t padded[136] = {};
    size_t remainder = len - offset;
    std::memcpy(padded, data + offset, remainder);
    padded[remainder] ^= delimiter;       // Keccak: 0x01
    padded[rate_bytes - 1] ^= 0x80;      // Last byte of rate

    for (size_t i = 0; i < rate_bytes / 8; ++i) {
        uint64_t lane = 0;
        for (int b = 0; b < 8; ++b)
            lane |= (uint64_t)padded[i*8 + b] << (8*b);
        state[i] ^= lane;
    }
    keccakF1600(state);

    // Squeeze 32 bytes
    std::array<uint8_t, 32> out;
    for (int i = 0; i < 4; ++i) { // 4 uint64s = 32 bytes
        for (int b = 0; b < 8; ++b)
            out[i*8 + b] = (state[i] >> (8*b)) & 0xff;
    }
    return out;
}

} // anonymous namespace

std::array<uint8_t, 32> keccak256(const uint8_t* data, size_t len) {
    // rate = 1088 bits (136 bytes), capacity = 512 bits, delimiter = 0x01 (Keccak, not SHA3)
    return keccakHash(data, len, 1088, 0x01);
}

std::array<uint8_t, 32> keccak256(const std::vector<uint8_t>& data) {
    return keccak256(data.data(), data.size());
}

} // namespace crypto
