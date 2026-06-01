#include "hex.h"
#include <stdexcept>

namespace encoding {

std::string toHex(const uint8_t* data, size_t len) {
    std::string hex;
    hex.reserve(len * 2);
    constexpr char hexChars[] = "0123456789abcdef";
    for (size_t i = 0; i < len; ++i) {
        hex.push_back(hexChars[(data[i] >> 4) & 0x0F]);
        hex.push_back(hexChars[data[i] & 0x0F]);
    }
    return hex;
}

std::string toHex(const std::vector<uint8_t>& data) {
    return toHex(data.data(), data.size());
}

std::vector<uint8_t> fromHex(const std::string& hex) {
    std::vector<uint8_t> bytes;
    if (hex.length() % 2 != 0) {
        throw std::invalid_argument("Hex string length must be even");
    }
    bytes.reserve(hex.length() / 2);
    for (size_t i = 0; i < hex.length(); i += 2) {
        uint8_t byte = 0;
        for (int j = 0; j < 2; ++j) {
            char c = hex[i + j];
            uint8_t val = 0;
            if (c >= '0' && c <= '9') val = c - '0';
            else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
            else throw std::invalid_argument("Invalid hex character");
            byte = (byte << 4) | val;
        }
        bytes.push_back(byte);
    }
    return bytes;
}

} // namespace encoding
