#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace encoding {
    // Bech32 (BIP-0173) for Bitcoin SegWit addresses
    std::string bech32Encode(const std::string& hrp, int witnessVersion, const std::vector<uint8_t>& witnessProgram);
    
    // Returns witness version and program if valid, empty if invalid
    bool bech32Decode(const std::string& bech, std::string& hrp, int& witnessVersion, std::vector<uint8_t>& witnessProgram);
}
