#include "bip32.h"
#include "../crypto/hmac.h"
#include "../crypto/sha256.h"
#include "../crypto/ripemd160.h"
#include <secp256k1.h>
#include <cstring>
#include <stdexcept>

namespace bip32 {

ExtendedKey masterKeyFromSeed(const std::vector<uint8_t>& seed) {
    const char* key = "Bitcoin seed";
    auto I = crypto::hmac_sha512(reinterpret_cast<const uint8_t*>(key), 12, seed.data(), seed.size());

    ExtendedKey master;
    std::memcpy(master.key.data(), I.data(), 32);
    std::memcpy(master.chainCode.data(), I.data() + 32, 32);
    master.isPrivate = true;
    master.depth = 0;
    master.childNumber = 0;
    master.fingerprint.fill(0);

    return master;
}

ExtendedKey deriveChildPrivate(const ExtendedKey& parent, uint32_t index) {
    if (!parent.isPrivate) {
        throw std::invalid_argument("Cannot derive private child from public parent");
    }

    std::vector<uint8_t> data;
    if (index >= 0x80000000) {
        // Hardened: 0x00 || parent_key || index
        data.push_back(0);
        data.insert(data.end(), parent.key.begin(), parent.key.end());
    } else {
        // Normal: parent_pubkey || index
        auto pubkey = getPublicKeyCompressed(parent.key);
        data.insert(data.end(), pubkey.begin(), pubkey.end());
    }

    data.push_back((index >> 24) & 0xFF);
    data.push_back((index >> 16) & 0xFF);
    data.push_back((index >> 8) & 0xFF);
    data.push_back(index & 0xFF);

    auto I = crypto::hmac_sha512(parent.chainCode.data(), 32, data.data(), data.size());

    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    std::array<uint8_t, 32> childKey;
    std::memcpy(childKey.data(), parent.key.data(), 32);

    if (!secp256k1_ec_seckey_tweak_add(ctx, childKey.data(), I.data())) {
        secp256k1_context_destroy(ctx);
        throw std::runtime_error("Invalid child key derived");
    }
    secp256k1_context_destroy(ctx);

    ExtendedKey child;
    child.key = childKey;
    std::memcpy(child.chainCode.data(), I.data() + 32, 32);
    child.isPrivate = true;
    child.depth = parent.depth + 1;
    child.childNumber = index;

    auto parentPub = getPublicKeyCompressed(parent.key);
    auto parentHash = crypto::hash160(parentPub.data(), parentPub.size());
    std::memcpy(child.fingerprint.data(), parentHash.data(), 4);

    return child;
}

std::array<uint8_t, 33> getPublicKeyCompressed(const std::array<uint8_t, 32>& privateKey) {
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_create(ctx, &pubkey, privateKey.data())) {
        secp256k1_context_destroy(ctx);
        throw std::invalid_argument("Invalid private key");
    }

    std::array<uint8_t, 33> result;
    size_t len = 33;
    secp256k1_ec_pubkey_serialize(ctx, result.data(), &len, &pubkey, SECP256K1_EC_COMPRESSED);
    secp256k1_context_destroy(ctx);
    return result;
}

std::array<uint8_t, 65> getPublicKeyUncompressed(const std::array<uint8_t, 32>& privateKey) {
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_create(ctx, &pubkey, privateKey.data())) {
        secp256k1_context_destroy(ctx);
        throw std::invalid_argument("Invalid private key");
    }

    std::array<uint8_t, 65> result;
    size_t len = 65;
    secp256k1_ec_pubkey_serialize(ctx, result.data(), &len, &pubkey, SECP256K1_EC_UNCOMPRESSED);
    secp256k1_context_destroy(ctx);
    return result;
}

std::pair<std::array<uint8_t, 32>, std::array<uint8_t, 33>> deriveBIP44(const std::vector<uint8_t>& seed, uint32_t coinType, uint32_t addressIndex) {
    auto master = masterKeyFromSeed(seed);
    auto purpose = deriveChildPrivate(master, 44 | 0x80000000);
    auto coin = deriveChildPrivate(purpose, coinType | 0x80000000);
    auto account = deriveChildPrivate(coin, 0 | 0x80000000);
    auto change = deriveChildPrivate(account, 0);
    auto child = deriveChildPrivate(change, addressIndex);

    return {child.key, getPublicKeyCompressed(child.key)};
}

} // namespace bip32
