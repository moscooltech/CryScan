#pragma once
#include <string>
#include <array>
#include <cstdint>
#include <vector>

namespace address {
    // Derive Solana Ed25519 keypair from seed using SLIP-0010 for Ed25519
    // Returns {private_key_32, public_key_32}
    std::pair<std::array<uint8_t, 32>, std::array<uint8_t, 32>> deriveSolanaKeypair(const std::vector<uint8_t>& seed, uint32_t accountIndex = 0);
    
    // Encode Solana public key as base58 (no version byte, just 32-byte pubkey)
    std::string solanaAddress(const std::array<uint8_t, 32>& publicKey);
}
