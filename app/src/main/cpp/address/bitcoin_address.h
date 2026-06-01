#pragma once
#include <string>
#include <array>
#include <cstdint>

namespace address {
    // P2PKH address (starts with '1')
    std::string bitcoinP2PKH(const std::array<uint8_t, 33>& compressedPubKey);
    // Bech32 SegWit v0 address (starts with 'bc1q')
    std::string bitcoinBech32(const std::array<uint8_t, 33>& compressedPubKey);
}
