#pragma once
#include <string>
#include <array>
#include <cstdint>

namespace address {
    // Ethereum address (0x + 40 hex chars, EIP-55 checksum)
    std::string ethereumAddress(const std::array<uint8_t, 65>& uncompressedPubKey);
    // Apply EIP-55 checksum to lowercase hex address
    std::string eip55Checksum(const std::string& lowerHexAddress);
}
