#pragma once
#include <array>
#include <vector>
#include <string>
#include <cstdint>

namespace bip32 {
    struct ExtendedKey {
        std::array<uint8_t, 32> key;        // private or public key
        std::array<uint8_t, 32> chainCode;
        bool isPrivate;
        uint8_t depth;
        uint32_t childNumber;
        std::array<uint8_t, 4> fingerprint;
    };
    
    // Derive master key from 64-byte seed
    ExtendedKey masterKeyFromSeed(const std::vector<uint8_t>& seed);
    
    // Derive child key (private)
    ExtendedKey deriveChildPrivate(const ExtendedKey& parent, uint32_t index);
    
    // Get public key from private key (secp256k1)
    std::array<uint8_t, 33> getPublicKeyCompressed(const std::array<uint8_t, 32>& privateKey);
    
    // Get uncompressed public key (for Ethereum)
    std::array<uint8_t, 65> getPublicKeyUncompressed(const std::array<uint8_t, 32>& privateKey);
    
    // Derive key at BIP-44 path: m/44'/coin_type'/0'/0/index
    // Returns {private_key_bytes, public_key_compressed_bytes}
    std::pair<std::array<uint8_t, 32>, std::array<uint8_t, 33>> deriveBIP44(const std::vector<uint8_t>& seed, uint32_t coinType, uint32_t addressIndex);
    
    // Coin types
    static constexpr uint32_t COIN_BITCOIN  = 0;
    static constexpr uint32_t COIN_ETHEREUM = 60;
    static constexpr uint32_t COIN_SOLANA   = 501;
}
