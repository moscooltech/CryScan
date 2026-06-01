#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace crypto {
    std::array<uint8_t, 32> keccak256(const uint8_t* data, size_t len);
    std::array<uint8_t, 32> keccak256(const std::vector<uint8_t>& data);
}
