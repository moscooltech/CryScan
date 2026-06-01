#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace encoding {
    std::string base58Encode(const std::vector<uint8_t>& data);
    std::vector<uint8_t> base58Decode(const std::string& encoded);
    std::string base58CheckEncode(const std::vector<uint8_t>& payload, uint8_t version);
    std::vector<uint8_t> base58CheckDecode(const std::string& encoded);
}
