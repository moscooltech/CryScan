#include "bech32.h"
#include <stdexcept>

namespace encoding {

namespace {

const char* CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

uint32_t polymod(const std::vector<uint8_t>& values) {
    uint32_t chk = 1;
    for (size_t i = 0; i < values.size(); ++i) {
        uint8_t top = chk >> 25;
        chk = (chk & 0x1ffffff) << 5 ^ values[i];
        if (top & 1) chk ^= 0x3b6a57b2;
        if (top & 2) chk ^= 0x26508e6d;
        if (top & 4) chk ^= 0x1ea119fa;
        if (top & 8) chk ^= 0x3d4233dd;
        if (top & 16) chk ^= 0x2a1462b3;
    }
    return chk;
}

std::vector<uint8_t> expandHrp(const std::string& hrp) {
    std::vector<uint8_t> ret;
    ret.reserve(hrp.size() * 2 + 1);
    for (size_t i = 0; i < hrp.size(); ++i) {
        ret.push_back(hrp[i] >> 5);
    }
    ret.push_back(0);
    for (size_t i = 0; i < hrp.size(); ++i) {
        ret.push_back(hrp[i] & 31);
    }
    return ret;
}

bool convertBits(std::vector<uint8_t>& out, const std::vector<uint8_t>& in, int fromBits, int toBits, bool pad) {
    int acc = 0;
    int bits = 0;
    int maxv = (1 << toBits) - 1;
    int max_acc = (1 << (fromBits + toBits - 1)) - 1;

    for (size_t i = 0; i < in.size(); ++i) {
        int value = in[i];
        if (value < 0 || (value >> fromBits) != 0) {
            return false;
        }
        acc = ((acc << fromBits) | value) & max_acc;
        bits += fromBits;
        while (bits >= toBits) {
            bits -= toBits;
            out.push_back((acc >> bits) & maxv);
        }
    }

    if (pad) {
        if (bits > 0) {
            out.push_back((acc << (toBits - bits)) & maxv);
        }
    } else if (bits >= fromBits || ((acc << (toBits - bits)) & maxv)) {
        return false;
    }

    return true;
}

} // namespace

std::string bech32Encode(const std::string& hrp, int witnessVersion, const std::vector<uint8_t>& witnessProgram) {
    std::vector<uint8_t> data;
    data.push_back(witnessVersion);

    std::vector<uint8_t> conv;
    if (!convertBits(conv, witnessProgram, 8, 5, true)) {
        return "";
    }
    data.insert(data.end(), conv.begin(), conv.end());

    std::vector<uint8_t> values = expandHrp(hrp);
    values.insert(values.end(), data.begin(), data.end());

    std::vector<uint8_t> zeroes = {0, 0, 0, 0, 0, 0};
    values.insert(values.end(), zeroes.begin(), zeroes.end());

    uint32_t polymod_val = polymod(values) ^ 1;

    std::string ret = hrp + "1";
    for (size_t i = 0; i < data.size(); ++i) {
        ret += CHARSET[data[i]];
    }

    for (int i = 0; i < 6; ++i) {
        ret += CHARSET[(polymod_val >> 5 * (5 - i)) & 31];
    }

    return ret;
}

bool bech32Decode(const std::string& bech, std::string& hrp, int& witnessVersion, std::vector<uint8_t>& witnessProgram) {
    // Simplified for this task, a full decode would do polymod check, separate HRP, etc.
    return false; // Stub implementation for decode, since we only need encode for wallet generation right now
}

} // namespace encoding
