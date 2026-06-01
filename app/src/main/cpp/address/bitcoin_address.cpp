#include "bitcoin_address.h"
#include "../crypto/ripemd160.h"
#include "../encoding/base58.h"
#include "../encoding/bech32.h"

namespace address {

std::string bitcoinP2PKH(const std::array<uint8_t, 33>& compressedPubKey) {
    auto hash160 = crypto::hash160(compressedPubKey.data(), compressedPubKey.size());
    std::vector<uint8_t> payload(hash160.begin(), hash160.end());
    return encoding::base58CheckEncode(payload, 0x00);
}

std::string bitcoinBech32(const std::array<uint8_t, 33>& compressedPubKey) {
    auto hash160 = crypto::hash160(compressedPubKey.data(), compressedPubKey.size());
    std::vector<uint8_t> payload(hash160.begin(), hash160.end());
    return encoding::bech32Encode("bc", 0, payload);
}

} // namespace address
