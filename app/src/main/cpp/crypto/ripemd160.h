#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace crypto {
    std::array<uint8_t, 20> ripemd160(const uint8_t* data, size_t len);
    std::array<uint8_t, 20> ripemd160(const std::vector<uint8_t>& data);
    std::array<uint8_t, 20> hash160(const uint8_t* data, size_t len); // SHA256 then RIPEMD160
}
