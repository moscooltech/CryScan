#include "ethereum_address.h"
#include "../crypto/keccak256.h"
#include "../encoding/hex.h"
#include <cctype>

namespace address {

std::string eip55Checksum(const std::string& lowerHexAddress) {
    std::string cleanAddr = lowerHexAddress;
    if (cleanAddr.substr(0, 2) == "0x") {
        cleanAddr = cleanAddr.substr(2);
    }

    auto hash = crypto::keccak256(reinterpret_cast<const uint8_t*>(cleanAddr.data()), cleanAddr.length());
    std::string hashHex = encoding::toHex(hash.data(), hash.size());

    std::string result = "0x";
    for (size_t i = 0; i < cleanAddr.length(); ++i) {
        char c = cleanAddr[i];
        if (c >= 'a' && c <= 'f') {
            int hashNibble;
            char hc = hashHex[i];
            if (hc >= '0' && hc <= '9') hashNibble = hc - '0';
            else hashNibble = hc - 'a' + 10;
            
            if (hashNibble >= 8) {
                result += std::toupper(c);
            } else {
                result += c;
            }
        } else {
            result += c;
        }
    }
    return result;
}

std::string ethereumAddress(const std::array<uint8_t, 65>& uncompressedPubKey) {
    // Drop the 0x04 prefix
    std::vector<uint8_t> pubKey(uncompressedPubKey.begin() + 1, uncompressedPubKey.end());
    auto hash = crypto::keccak256(pubKey);
    
    // Take the last 20 bytes
    std::vector<uint8_t> addressBytes(hash.end() - 20, hash.end());
    std::string hexAddr = encoding::toHex(addressBytes);
    
    return eip55Checksum(hexAddr);
}

} // namespace address
