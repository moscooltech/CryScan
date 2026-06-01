// Complete RIPEMD-160 implementation
// Reference: https://homes.esat.kuleuven.be/~bosMDlen/ripemd160.html
// Test vector: ripemd160("abc") = 8eb208f7e05d987a9b044a8e98c6b087f15a0bfc
// hash160("") = b472a266d0bd89c13706a4132ccfb16f7c3b9fcb (SHA256 then RIPEMD160)
#include "ripemd160.h"
#include "sha256.h"
#include <cstring>

namespace crypto {

namespace {

inline uint32_t rotr_r160(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }
inline uint32_t le32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
inline void store_le32(uint8_t* p, uint32_t v) {
    p[0]=v&0xff; p[1]=(v>>8)&0xff; p[2]=(v>>16)&0xff; p[3]=(v>>24)&0xff;
}

// RIPEMD-160 nonlinear functions
inline uint32_t f1(uint32_t x,uint32_t y,uint32_t z) { return x^y^z; }
inline uint32_t f2(uint32_t x,uint32_t y,uint32_t z) { return (x&y)|(~x&z); }
inline uint32_t f3(uint32_t x,uint32_t y,uint32_t z) { return (x|~y)^z; }
inline uint32_t f4(uint32_t x,uint32_t y,uint32_t z) { return (x&z)|(y&~z); }
inline uint32_t f5(uint32_t x,uint32_t y,uint32_t z) { return x^(y|~z); }

// Round constants
static constexpr uint32_t KL[5] = {0x00000000, 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xA953FD4E};
static constexpr uint32_t KR[5] = {0x50A28BE6, 0x5C4DD124, 0x6D703EF3, 0x7A6D76E9, 0x00000000};

// Message word selection (left rounds)
static constexpr int RL[80] = {
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
    7,4,13,1,10,6,15,3,12,0,9,5,2,14,11,8,
    3,10,14,4,9,15,8,1,2,7,0,6,13,11,5,12,
    1,9,11,10,0,8,12,4,13,3,7,15,14,5,6,2,
    4,0,5,9,7,12,2,10,14,1,3,8,11,6,15,13
};

// Message word selection (right rounds)
static constexpr int RR[80] = {
    5,14,7,0,9,2,11,4,13,6,15,8,1,10,3,12,
    6,11,3,7,0,13,5,10,14,15,8,12,4,9,1,2,
    15,5,1,3,7,14,6,9,11,8,12,2,10,0,4,13,
    8,6,4,1,3,11,15,0,5,12,2,13,9,7,10,14,
    12,15,10,4,1,5,8,7,6,2,13,14,0,3,9,11
};

// Shift amounts (left)
static constexpr int SL[80] = {
    11,14,15,12,5,8,7,9,11,13,14,15,6,7,9,8,
    7,6,8,13,11,9,7,15,7,12,15,9,11,7,13,12,
    11,13,6,7,14,9,13,15,14,8,13,6,5,12,7,5,
    11,12,14,15,14,15,9,8,9,14,5,6,8,6,5,12,
    9,15,5,11,6,8,13,12,5,12,13,14,11,8,5,6
};

// Shift amounts (right)
static constexpr int SR[80] = {
    8,9,9,11,13,15,15,5,7,7,8,11,14,14,12,6,
    9,13,15,7,12,8,9,11,7,7,12,7,6,15,13,11,
    9,7,15,11,8,6,6,14,12,13,5,14,13,13,7,5,
    15,5,8,11,14,14,6,14,6,9,12,9,12,5,15,8,
    8,5,12,9,12,5,14,6,8,13,6,5,15,13,11,11
};

struct RipeMD160State {
    uint32_t h[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};

    void processBlock(const uint8_t block[64]) {
        uint32_t X[16];
        for (int i = 0; i < 16; ++i) X[i] = le32(block + i*4);

        uint32_t al=h[0],bl=h[1],cl=h[2],dl=h[3],el=h[4];
        uint32_t ar=h[0],br=h[1],cr=h[2],dr=h[3],er=h[4];

        for (int i = 0; i < 80; ++i) {
            int round = i / 16;
            uint32_t fl, fr;
            switch (round) {
                case 0: fl=f1(bl,cl,dl); fr=f5(br,cr,dr); break;
                case 1: fl=f2(bl,cl,dl); fr=f4(br,cr,dr); break;
                case 2: fl=f3(bl,cl,dl); fr=f3(br,cr,dr); break;
                case 3: fl=f4(bl,cl,dl); fr=f2(br,cr,dr); break;
                default:fl=f5(bl,cl,dl); fr=f1(br,cr,dr); break;
            }
            uint32_t T;
            // Left
            T = rotr_r160(al + fl + X[RL[i]] + KL[round], SL[i]) + el;
            al=el; el=dl; dl=rotr_r160(cl,10); cl=bl; bl=T;
            // Right
            T = rotr_r160(ar + fr + X[RR[i]] + KR[round], SR[i]) + er;
            ar=er; er=dr; dr=rotr_r160(cr,10); cr=br; br=T;
        }

        uint32_t T = h[1] + cl + dr;
        h[1] = h[2] + dl + er;
        h[2] = h[3] + el + ar;
        h[3] = h[4] + al + br;
        h[4] = h[0] + bl + cr;
        h[0] = T;
    }

    std::array<uint8_t, 20> digest() const {
        std::array<uint8_t, 20> out;
        for (int i = 0; i < 5; ++i)
            store_le32(out.data() + i*4, h[i]);
        return out;
    }
};

} // anonymous namespace

std::array<uint8_t, 20> ripemd160(const uint8_t* data, size_t len) {
    RipeMD160State state;

    size_t offset = 0;
    while (offset + 64 <= len) {
        state.processBlock(data + offset);
        offset += 64;
    }

    // Padding (little-endian length, same bit-count scheme as MD4/MD5)
    uint8_t block[128] = {};
    size_t remainder = len - offset;
    std::memcpy(block, data + offset, remainder);
    block[remainder] = 0x80;

    uint64_t bitlen = (uint64_t)len * 8;

    if (remainder < 56) {
        // Little-endian 64-bit length
        for (int i = 0; i < 8; ++i)
            block[56 + i] = (uint8_t)(bitlen >> (8 * i));
        state.processBlock(block);
    } else {
        for (int i = 0; i < 8; ++i)
            block[120 + i] = (uint8_t)(bitlen >> (8 * i));
        state.processBlock(block);
        state.processBlock(block + 64);
    }

    return state.digest();
}

std::array<uint8_t, 20> ripemd160(const std::vector<uint8_t>& data) {
    return ripemd160(data.data(), data.size());
}

std::array<uint8_t, 20> hash160(const uint8_t* data, size_t len) {
    auto sha = sha256(data, len);
    return ripemd160(sha.data(), sha.size());
}

} // namespace crypto
