#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <string>

namespace crypto {
    std::array<uint8_t, 32> sha256(const uint8_t* data, size_t len);
    std::array<uint8_t, 32> sha256(const std::vector<uint8_t>& data);
    std::array<uint8_t, 32> sha256d(const uint8_t* data, size_t len); // double SHA-256
}
