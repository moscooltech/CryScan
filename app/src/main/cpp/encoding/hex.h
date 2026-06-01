#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <array>

namespace encoding {
    std::string toHex(const uint8_t* data, size_t len);
    std::string toHex(const std::vector<uint8_t>& data);

    template<size_t N>
    std::string toHex(const std::array<uint8_t, N>& data) {
        return toHex(data.data(), data.size());
    }

    std::vector<uint8_t> fromHex(const std::string& hex);
}
