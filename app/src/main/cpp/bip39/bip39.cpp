#include "bip39.h"
#include "wordlist_english.h"
#include "../crypto/sha256.h"
#include "../crypto/hmac.h"
#include <sstream>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>

#if defined(__linux__) || defined(__ANDROID__)
#include <sys/random.h>
#endif

namespace bip39 {

std::vector<std::string> splitMnemonic(const std::string& mnemonic) {
    std::vector<std::string> words;
    std::istringstream iss(mnemonic);
    std::string word;
    while (iss >> word) {
        words.push_back(word);
    }
    return words;
}

std::vector<uint8_t> generateEntropy(WordCount wordCount) {
    size_t entropyBytes = static_cast<int>(wordCount) * 4 / 3;
    std::vector<uint8_t> entropy(entropyBytes);

#if defined(__linux__) || defined(__ANDROID__)
    // Try getrandom syscall first
    if (syscall(SYS_getrandom, entropy.data(), entropyBytes, 0) == static_cast<ssize_t>(entropyBytes)) {
        return entropy;
    }
#endif

    // Fallback to /dev/urandom
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        ssize_t bytesRead = read(fd, entropy.data(), entropyBytes);
        close(fd);
        if (bytesRead == static_cast<ssize_t>(entropyBytes)) {
            return entropy;
        }
    }

    throw std::runtime_error("Failed to generate cryptographically secure entropy");
}

std::string entropyToMnemonic(const std::vector<uint8_t>& entropy) {
    if (entropy.size() % 4 != 0 || entropy.size() < 16 || entropy.size() > 32) {
        throw std::invalid_argument("Entropy must be 16-32 bytes and a multiple of 4 bytes");
    }

    auto hash = crypto::sha256(entropy);
    
    // Convert entropy + checksum into bits
    std::vector<bool> bits;
    bits.reserve(entropy.size() * 8 + entropy.size() * 8 / 32);

    for (uint8_t b : entropy) {
        for (int i = 7; i >= 0; --i) {
            bits.push_back((b >> i) & 1);
        }
    }

    // Append checksum
    size_t checksumLen = entropy.size() / 4;
    for (size_t i = 0; i < checksumLen; ++i) {
        bits.push_back((hash[0] >> (7 - i)) & 1);
    }

    std::string mnemonic;
    for (size_t i = 0; i < bits.size(); i += 11) {
        int index = 0;
        for (int j = 0; j < 11; ++j) {
            index <<= 1;
            if (bits[i + j]) {
                index |= 1;
            }
        }
        mnemonic += ENGLISH_WORDLIST[index];
        if (i + 11 < bits.size()) {
            mnemonic += " ";
        }
    }

    return mnemonic;
}

std::string generateMnemonic(WordCount wordCount) {
    return entropyToMnemonic(generateEntropy(wordCount));
}

bool validateMnemonic(const std::string& mnemonic) {
    auto words = splitMnemonic(mnemonic);
    if (words.size() % 3 != 0 || words.size() < 12 || words.size() > 24) {
        return false;
    }

    std::vector<bool> bits;
    bits.reserve(words.size() * 11);

    for (const auto& word : words) {
        auto it = std::find(ENGLISH_WORDLIST.begin(), ENGLISH_WORDLIST.end(), word);
        if (it == ENGLISH_WORDLIST.end()) return false;
        
        int index = std::distance(ENGLISH_WORDLIST.begin(), it);
        for (int i = 10; i >= 0; --i) {
            bits.push_back((index >> i) & 1);
        }
    }

    size_t checksumLen = words.size() / 3;
    size_t entropyLen = bits.size() - checksumLen;

    std::vector<uint8_t> entropy(entropyLen / 8, 0);
    for (size_t i = 0; i < entropyLen; ++i) {
        if (bits[i]) {
            entropy[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    auto hash = crypto::sha256(entropy);
    
    for (size_t i = 0; i < checksumLen; ++i) {
        bool bit = (hash[0] >> (7 - i)) & 1;
        if (bit != bits[entropyLen + i]) return false;
    }

    return true;
}

std::vector<uint8_t> mnemonicToSeed(const std::string& mnemonic, const std::string& passphrase) {
    std::string salt_str = "mnemonic" + passphrase;
    std::vector<uint8_t> salt(salt_str.begin(), salt_str.end());
    return crypto::pbkdf2_hmac_sha512(mnemonic, salt, 2048, 64);
}

} // namespace bip39
