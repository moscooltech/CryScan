// BIP-32 / BIP-44 Unit Tests
// Test vectors from: https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#test-vectors

#include <gtest/gtest.h>
#include "bip32/bip32.h"
#include "encoding/hex.h"
#include <string>
#include <vector>

using namespace bip32;
using namespace encoding;

// ─── BIP-32 Test Vector 1 ─────────────────────────────────────────────────────
// Seed: 000102030405060708090a0b0c0d0e0f

TEST(BIP32Test, Vector1_MasterKey) {
    auto seed = fromHex("000102030405060708090a0b0c0d0e0f");
    auto master = masterKeyFromSeed(seed);
    
    EXPECT_EQ(toHex(master.key),
        "e8f32e723decf4051aefac8e2c93c9c5b214313817cdb01a1494b917c8436b35");
    EXPECT_EQ(toHex(master.chainCode),
        "873dff81c02f525623fd1fe5167eac3a55a049de3d314bb42ee227ffed37d508");
    EXPECT_TRUE(master.isPrivate);
    EXPECT_EQ(master.depth, 0u);
}

TEST(BIP32Test, Vector1_ChildM0H) {
    // m/0' (hardened)
    auto seed = fromHex("000102030405060708090a0b0c0d0e0f");
    auto master = masterKeyFromSeed(seed);
    auto child = deriveChildPrivate(master, 0x80000000); // 0'
    
    EXPECT_EQ(toHex(child.key),
        "edb2e14f9ee77d26dd93b4fadece2ac5be7b4b11a5e0a85ccdbc9cdb7d91c03f");
    EXPECT_EQ(toHex(child.chainCode),
        "47fdacbd0f1097043b78c63c20c34ef4ed9a111d980047ad16282c7ae6236141");
}

TEST(BIP32Test, Vector1_ChildM0H1) {
    // m/0'/1 (normal)
    auto seed = fromHex("000102030405060708090a0b0c0d0e0f");
    auto master = masterKeyFromSeed(seed);
    auto child0h = deriveChildPrivate(master, 0x80000000);
    auto child0h1 = deriveChildPrivate(child0h, 1);
    
    EXPECT_EQ(toHex(child0h1.key),
        "3c6cb8d0f6a264c91ea8b5030fadaa8e538b020f0a387421a12de9319dc93368");
    EXPECT_EQ(toHex(child0h1.chainCode),
        "2a7857631386ba23dacac34180dd1983734e444fdbf774041578e9b6adb37c19");
}

// ─── BIP-32 Test Vector 2 ─────────────────────────────────────────────────────
// Seed: fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c9996...

TEST(BIP32Test, Vector2_MasterKey) {
    auto seed = fromHex(
        "fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a2"
        "9f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542"
    );
    auto master = masterKeyFromSeed(seed);
    
    EXPECT_EQ(toHex(master.key),
        "4b03d6fc340455b363f51020ad3ecca4f0850280cf436c70c727923f6db46c3e");
    EXPECT_EQ(toHex(master.chainCode),
        "60499f801b896d83179a4374aeb7822aaeaceaa0db1f85ee3e904c4defbd9689");
}

// ─── BIP-44 Derivation Tests ──────────────────────────────────────────────────

TEST(BIP44Test, BitcoinDerivation_KnownSeed) {
    // Using "abandon" x11 + "about" mnemonic, empty passphrase
    // BIP-39 seed for this mnemonic (from official vectors)
    auto seed = fromHex(
        "5eb00bbddcf069084889a8ab9155568165f5c453ccb85e70811aaed6f6da5fc1"
        "9a5ac40b389cd370d086206dec8aa6c43daea6690f20ad3d8d48b2d2ce9e38e4"
    );
    
    auto [privKey, pubKey] = deriveBIP44(seed, COIN_BITCOIN, 0);
    
    // m/44'/0'/0'/0/0 for this seed
    EXPECT_EQ(privKey.size(), 32u);
    EXPECT_EQ(pubKey.size(), 33u);
    
    // Public key should start with 0x02 or 0x03 (compressed)
    EXPECT_TRUE(pubKey[0] == 0x02 || pubKey[0] == 0x03);
}

TEST(BIP44Test, EthereumDerivation_KnownSeed) {
    auto seed = fromHex(
        "5eb00bbddcf069084889a8ab9155568165f5c453ccb85e70811aaed6f6da5fc1"
        "9a5ac40b389cd370d086206dec8aa6c43daea6690f20ad3d8d48b2d2ce9e38e4"
    );
    
    auto [privKey, pubKey] = deriveBIP44(seed, COIN_ETHEREUM, 0);
    
    EXPECT_EQ(privKey.size(), 32u);
    EXPECT_EQ(pubKey.size(), 33u);
    
    // Known Ethereum address for abandon...about mnemonic: 0x9858EfFD232B4033E47d90003D41EC34EcaEdA94
    // (from MetaMask reference implementation)
}

TEST(BIP44Test, MultipleIndexes_AreDifferent) {
    auto seed = fromHex(
        "5eb00bbddcf069084889a8ab9155568165f5c453ccb85e70811aaed6f6da5fc1"
        "9a5ac40b389cd370d086206dec8aa6c43daea6690f20ad3d8d48b2d2ce9e38e4"
    );
    
    auto [priv0, pub0] = deriveBIP44(seed, COIN_BITCOIN, 0);
    auto [priv1, pub1] = deriveBIP44(seed, COIN_BITCOIN, 1);
    auto [priv2, pub2] = deriveBIP44(seed, COIN_BITCOIN, 2);
    
    // All private keys should be different
    EXPECT_NE(toHex(priv0), toHex(priv1));
    EXPECT_NE(toHex(priv1), toHex(priv2));
    EXPECT_NE(toHex(priv0), toHex(priv2));
}

TEST(BIP44Test, DifferentCoins_ProduceDifferentKeys) {
    auto seed = fromHex(
        "5eb00bbddcf069084889a8ab9155568165f5c453ccb85e70811aaed6f6da5fc1"
        "9a5ac40b389cd370d086206dec8aa6c43daea6690f20ad3d8d48b2d2ce9e38e4"
    );
    
    auto [btcPriv, btcPub] = deriveBIP44(seed, COIN_BITCOIN, 0);
    auto [ethPriv, ethPub] = deriveBIP44(seed, COIN_ETHEREUM, 0);
    auto [solPriv, solPub] = deriveBIP44(seed, COIN_SOLANA, 0);
    
    EXPECT_NE(toHex(btcPriv), toHex(ethPriv));
    EXPECT_NE(toHex(ethPriv), toHex(solPriv));
    EXPECT_NE(toHex(btcPriv), toHex(solPriv));
}

// ─── secp256k1 Public Key Tests ───────────────────────────────────────────────

TEST(Secp256k1Test, CompressedPublicKey_StartsWith02Or03) {
    // Known private key: 0x1234...
    std::array<uint8_t, 32> privKey;
    privKey.fill(0);
    privKey[31] = 0x01; // private key = 1 (valid)
    
    auto pubKey = getPublicKeyCompressed(privKey);
    EXPECT_EQ(pubKey.size(), 33u);
    EXPECT_TRUE(pubKey[0] == 0x02 || pubKey[0] == 0x03);
}

TEST(Secp256k1Test, UncompressedPublicKey_StartsWith04) {
    std::array<uint8_t, 32> privKey;
    privKey.fill(0);
    privKey[31] = 0x01;
    
    auto pubKey = getPublicKeyUncompressed(privKey);
    EXPECT_EQ(pubKey.size(), 65u);
    EXPECT_EQ(pubKey[0], 0x04);
}

TEST(Secp256k1Test, Deterministic) {
    std::array<uint8_t, 32> privKey;
    privKey.fill(0x42);
    
    auto pub1 = getPublicKeyCompressed(privKey);
    auto pub2 = getPublicKeyCompressed(privKey);
    EXPECT_EQ(pub1, pub2);
}
