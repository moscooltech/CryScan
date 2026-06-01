#include "base58.h"
#include "../crypto/sha256.h"
#include <stdexcept>
#include <cstring>
#include <algorithm>

namespace encoding {

static const char* ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

std::string base58Encode(const std::vector<uint8_t>& data) {
    if (data.empty()) return "";

    size_t zeroes = 0;
    while (zeroes < data.size() && data[zeroes] == 0) {
        zeroes++;
    }

    size_t size = (data.size() - zeroes) * 138 / 100 + 1;
    std::vector<uint8_t> b58(size, 0);

    size_t length = 0;
    for (size_t i = zeroes; i < data.size(); i++) {
        uint32_t carry = data[i];
        size_t j = 0;
        for (std::vector<uint8_t>::reverse_iterator it = b58.rbegin(); 
             (carry != 0 || j < length) && (it != b58.rend()); 
             ++it, ++j) {
            carry += 256 * (*it);
            *it = carry % 58;
            carry /= 58;
        }
        length = j;
    }

    auto it = b58.begin() + (size - length);
    std::string result(zeroes, '1');
    while (it != b58.end()) {
        result += ALPHABET[*it];
        ++it;
    }

    return result;
}

std::vector<uint8_t> base58Decode(const std::string& encoded) {
    if (encoded.empty()) return {};

    size_t zeroes = 0;
    while (zeroes < encoded.length() && encoded[zeroes] == '1') {
        zeroes++;
    }

    size_t size = (encoded.length() - zeroes) * 733 / 1000 + 1;
    std::vector<uint8_t> b256(size, 0);

    size_t length = 0;
    for (size_t i = zeroes; i < encoded.length(); i++) {
        const char* p = strchr(ALPHABET, encoded[i]);
        if (p == nullptr) {
            throw std::invalid_argument("Invalid base58 character");
        }
        uint32_t carry = p - ALPHABET;
        size_t j = 0;
        for (std::vector<uint8_t>::reverse_iterator it = b256.rbegin(); 
             (carry != 0 || j < length) && (it != b256.rend()); 
             ++it, ++j) {
            carry += 58 * (*it);
            *it = carry % 256;
            carry /= 256;
        }
        length = j;
    }

    auto it = b256.begin() + (size - length);
    std::vector<uint8_t> result(zeroes, 0x00);
    while (it != b256.end()) {
        result.push_back(*it);
        ++it;
    }

    return result;
}

std::string base58CheckEncode(const std::vector<uint8_t>& payload, uint8_t version) {
    std::vector<uint8_t> data;
    data.reserve(payload.size() + 5);
    data.push_back(version);
    data.insert(data.end(), payload.begin(), payload.end());

    auto hash1 = crypto::sha256(data);
    auto hash2 = crypto::sha256(hash1.data(), hash1.size());

    data.insert(data.end(), hash2.begin(), hash2.begin() + 4);

    return base58Encode(data);
}

std::vector<uint8_t> base58CheckDecode(const std::string& encoded) {
    std::vector<uint8_t> data = base58Decode(encoded);
    if (data.size() < 4) {
        throw std::invalid_argument("Base58Check data too short");
    }

    std::vector<uint8_t> payload(data.begin(), data.end() - 4);
    auto hash1 = crypto::sha256(payload);
    auto hash2 = crypto::sha256(hash1.data(), hash1.size());

    for (int i = 0; i < 4; ++i) {
        if (data[data.size() - 4 + i] != hash2[i]) {
            throw std::invalid_argument("Base58Check checksum mismatch");
        }
    }

    // Return version + payload
    return payload;
}

} // namespace encoding
