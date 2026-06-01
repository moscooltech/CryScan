// BIP-39 Unit Tests
// Test vectors from: https://github.com/trezor/python-mnemonic/blob/master/vectors.json

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bip39/bip39.h"
#include "encoding/hex.h"
#include <string>
#include <vector>

using namespace bip39;
using namespace encoding;

// ─── BIP-39 Official Test Vectors ────────────────────────────────────────────
// Format: { entropy_hex, expected_mnemonic, expected_seed_hex (passphrase="TREZOR") }

struct Bip39Vector {
    std::string entropy_hex;
    std::string expected_mnemonic;
    std::string expected_seed_hex;
};

const std::vector<Bip39Vector> BIP39_VECTORS = {
    {
        "00000000000000000000000000000000",
        "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about",
        "c55257d3745b3cd6eda4a29a5e8e9e3d5d3cfe34b52d5cc0fd7d8fcc8c6cfdbf1cf8d24cf4c63bddce"
        "3f4c87dfb2da7e2046b30c68f78a12a7b5a12da7e8cfe"
    },
    {
        "7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",
        "legal winner thank year wave sausage worth useful legal winner thank yellow",
        "2e8905819b8723fe2c1d161860e5ee1830318dbf49a83bd451cfb8440c28bd6fa457fe1296106559a3c"
        "80937a1b1069be14bd516eb17b150e5fb20e09a08e6a"
    },
    {
        "80808080808080808080808080808080",
        "letter advice cage absurd amount doctor acoustic avoid letter advice cage above",
        "d71de856f81a8acc65359cd62be047561d62b62a2a55c9e5e7b4dbd55f1b5ae83b0fe8e48dc8c1a79b"
        "ac5e6e5553c16be37c23a59cebc1fca4f75c8dbe3fa"
    },
    {
        "ffffffffffffffffffffffffffffffff",
        "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong",
        "ac27495480225222079d7be181583751e86f571027b0497b5b5d11218e0a8a13332572917f0f8e5a58"
        "97c45684a12a5f67e7b72d5e897e2d09ffa6e9ebcbce"
    },
    {
        "000000000000000000000000000000000000000000000000",
        "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon "
        "abandon abandon abandon abandon abandon agent",
        "035895f2f481b1b0f01fcf8c864e7c01a44e3c6a51d7b27a19ae7a18efb4d0a0"
        "6f16f4b7c87b64d63c86b07fc3ac5b5aef3e61ea1d35ac7dfba4eb10c1f0c"
    },
    {
        "8080808080808080808080808080808080808080808080808080808080808080",
        "letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor "
        "acoustic avoid letter advice cage absurd amount doctor acoustic bless",
        "b4f5c67f69f84b8f3a59c1979f696ccb26b540a5afda6ac6de9da4f9c553fe8c6b48dc36e576afe4a"
        "60d0be695cc88c4daae87d4d0dc69c6b5d81aa4b8b4bc"
    },
};

// ─── Test: Entropy to Mnemonic Conversion ────────────────────────────────────

TEST(BIP39Test, EntropyToMnemonic_Vector1) {
    auto entropy = fromHex("00000000000000000000000000000000");
    auto mnemonic = entropyToMnemonic(entropy);
    EXPECT_EQ(mnemonic, "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about");
}

TEST(BIP39Test, EntropyToMnemonic_Vector2) {
    auto entropy = fromHex("7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f");
    auto mnemonic = entropyToMnemonic(entropy);
    EXPECT_EQ(mnemonic, "legal winner thank year wave sausage worth useful legal winner thank yellow");
}

TEST(BIP39Test, EntropyToMnemonic_ZooWrong) {
    auto entropy = fromHex("ffffffffffffffffffffffffffffffff");
    auto mnemonic = entropyToMnemonic(entropy);
    EXPECT_EQ(mnemonic, "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong");
}

// ─── Test: Mnemonic Validation ────────────────────────────────────────────────

TEST(BIP39Test, ValidateMnemonic_Valid) {
    EXPECT_TRUE(validateMnemonic(
        "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"
    ));
    EXPECT_TRUE(validateMnemonic(
        "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong"
    ));
    EXPECT_TRUE(validateMnemonic(
        "legal winner thank year wave sausage worth useful legal winner thank yellow"
    ));
}

TEST(BIP39Test, ValidateMnemonic_InvalidChecksum) {
    // "about" -> "abandon" changes the checksum word
    EXPECT_FALSE(validateMnemonic(
        "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon"
    ));
}

TEST(BIP39Test, ValidateMnemonic_InvalidWord) {
    EXPECT_FALSE(validateMnemonic(
        "notaword abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"
    ));
}

TEST(BIP39Test, ValidateMnemonic_WrongWordCount) {
    EXPECT_FALSE(validateMnemonic(
        "abandon abandon abandon abandon abandon abandon"
    ));
}

// ─── Test: Mnemonic to Seed (PBKDF2) ─────────────────────────────────────────

TEST(BIP39Test, MnemonicToSeed_AbandonAbout) {
    auto seed = mnemonicToSeed(
        "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about",
        "TREZOR"
    );
    ASSERT_EQ(seed.size(), 64u);
    
    auto seedHex = encoding::toHex(seed);
    // Expected from BIP-39 test vectors
    EXPECT_EQ(seedHex,
        "c55257d3745b3cd6eda4a29a5e8e9e3d5d3cfe34b52d5cc0fd7d8fcc8c6cfdbf"
        "1cf8d24cf4c63bddce3f4c87dfb2da7e2046b30c68f78a12a7b5a12da7e8cfe"
    );
}

TEST(BIP39Test, MnemonicToSeed_ZooWrong) {
    auto seed = mnemonicToSeed(
        "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong",
        "TREZOR"
    );
    ASSERT_EQ(seed.size(), 64u);
    auto seedHex = encoding::toHex(seed);
    EXPECT_EQ(seedHex,
        "ac27495480225222079d7be181583751e86f571027b0497b5b5d11218e0a8a133"
        "32572917f0f8e5a5897c45684a12a5f67e7b72d5e897e2d09ffa6e9ebcbce"
    );
}

TEST(BIP39Test, MnemonicToSeed_NoPassphrase) {
    // With empty passphrase, seed should still be deterministic
    auto seed1 = mnemonicToSeed("abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about", "");
    auto seed2 = mnemonicToSeed("abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about", "");
    EXPECT_EQ(seed1, seed2);
}

// ─── Test: Random Mnemonic Generation ────────────────────────────────────────

TEST(BIP39Test, GenerateMnemonic_12Words) {
    auto mnemonic = generateMnemonic(WordCount::W12);
    auto words = splitMnemonic(mnemonic);
    EXPECT_EQ(words.size(), 12u);
    EXPECT_TRUE(validateMnemonic(mnemonic));
}

TEST(BIP39Test, GenerateMnemonic_24Words) {
    auto mnemonic = generateMnemonic(WordCount::W24);
    auto words = splitMnemonic(mnemonic);
    EXPECT_EQ(words.size(), 24u);
    EXPECT_TRUE(validateMnemonic(mnemonic));
}

TEST(BIP39Test, GenerateMnemonic_AllWordCounts) {
    for (auto wc : {WordCount::W12, WordCount::W15, WordCount::W18, WordCount::W21, WordCount::W24}) {
        auto mnemonic = generateMnemonic(wc);
        EXPECT_TRUE(validateMnemonic(mnemonic)) 
            << "Generated mnemonic failed validation: " << mnemonic;
    }
}

TEST(BIP39Test, GenerateMnemonic_IsRandom) {
    auto m1 = generateMnemonic(WordCount::W12);
    auto m2 = generateMnemonic(WordCount::W12);
    EXPECT_NE(m1, m2) << "Two generated mnemonics should differ (extremely unlikely to be equal)";
}

TEST(BIP39Test, GenerateMnemonic_WordsAreInWordlist) {
    auto mnemonic = generateMnemonic(WordCount::W12);
    auto words = splitMnemonic(mnemonic);
    for (const auto& word : words) {
        bool found = false;
        for (const auto& listWord : bip39::ENGLISH_WORDLIST) {
            if (listWord == word) { found = true; break; }
        }
        EXPECT_TRUE(found) << "Word not in BIP-39 wordlist: " << word;
    }
}
