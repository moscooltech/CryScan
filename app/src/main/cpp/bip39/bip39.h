#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace bip39 {
    enum class WordCount { W12 = 12, W15 = 15, W18 = 18, W21 = 21, W24 = 24 };
    
    // Generate cryptographically secure entropy
    std::vector<uint8_t> generateEntropy(WordCount wordCount);
    
    // Convert entropy bytes to mnemonic phrase
    std::string entropyToMnemonic(const std::vector<uint8_t>& entropy);
    
    // Generate a complete random mnemonic
    std::string generateMnemonic(WordCount wordCount = WordCount::W12);
    
    // Validate a mnemonic phrase (checksum verification)
    bool validateMnemonic(const std::string& mnemonic);
    
    // Convert mnemonic + optional passphrase to 64-byte seed (PBKDF2-HMAC-SHA512)
    std::vector<uint8_t> mnemonicToSeed(const std::string& mnemonic, const std::string& passphrase = "");
    
    // Split mnemonic into words
    std::vector<std::string> splitMnemonic(const std::string& mnemonic);
}
