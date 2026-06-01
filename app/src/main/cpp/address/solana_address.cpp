#include "solana_address.h"
#include "../crypto/hmac.h"
#include "../encoding/base58.h"
#include "../third_party/ed25519/src/ed25519.h"
#include <cstring>

namespace address {

static void hmac_sha512_slip0010(const uint8_t* key, size_t key_len, const uint8_t* data, size_t data_len, std::array<uint8_t, 64>& out) {
    out = crypto::hmac_sha512(key, key_len, data, data_len);
}

std::pair<std::array<uint8_t, 32>, std::array<uint8_t, 32>> deriveSolanaKeypair(const std::vector<uint8_t>& seed, uint32_t accountIndex) {
    // SLIP-0010 Master Key Derivation for Ed25519
    const char* curve = "ed25519 seed";
    std::array<uint8_t, 64> I;
    hmac_sha512_slip0010(reinterpret_cast<const uint8_t*>(curve), 12, seed.data(), seed.size(), I);
    
    std::array<uint8_t, 32> parentKey;
    std::array<uint8_t, 32> parentChainCode;
    std::memcpy(parentKey.data(), I.data(), 32);
    std::memcpy(parentChainCode.data(), I.data() + 32, 32);

    // Hardened derivation paths for Solana: m/44'/501'/accountIndex'/0'
    uint32_t path[] = {
        44 | 0x80000000,
        501 | 0x80000000,
        accountIndex | 0x80000000,
        0 | 0x80000000
    };

    for (uint32_t p : path) {
        std::vector<uint8_t> data;
        data.push_back(0x00);
        data.insert(data.end(), parentKey.begin(), parentKey.end());
        data.push_back((p >> 24) & 0xFF);
        data.push_back((p >> 16) & 0xFF);
        data.push_back((p >> 8) & 0xFF);
        data.push_back(p & 0xFF);

        hmac_sha512_slip0010(parentChainCode.data(), 32, data.data(), data.size(), I);
        std::memcpy(parentKey.data(), I.data(), 32);
        std::memcpy(parentChainCode.data(), I.data() + 32, 32);
    }

    // parentKey is now the 32-byte Ed25519 seed.
    std::array<uint8_t, 32> pubKey;
    uint8_t fullPrivKey[64];
    ed25519_create_keypair(pubKey.data(), fullPrivKey, parentKey.data());

    return {parentKey, pubKey};
}

std::string solanaAddress(const std::array<uint8_t, 32>& publicKey) {
    std::vector<uint8_t> pubKeyVec(publicKey.begin(), publicKey.end());
    return encoding::base58Encode(pubKeyVec);
}

} // namespace address
