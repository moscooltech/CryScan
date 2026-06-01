#include <jni.h>
#include <string>
#include <vector>
#include <android/log.h>
// Include all our modules
#include "../bip39/bip39.h"
#include "../bip32/bip32.h"
#include "../address/bitcoin_address.h"
#include "../address/ethereum_address.h"
#include "../address/solana_address.h"
#include "../encoding/base58.h"
#include "../encoding/hex.h"
#include <nlohmann/json.hpp>

#define LOG_TAG "CryptoScanner"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

using json = nlohmann::json;

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_cryptoscanner_NativeBridge_generateMnemonic(JNIEnv* env, jclass, jint wordCount) {
    try {
        bip39::WordCount wc = static_cast<bip39::WordCount>(wordCount);
        std::string mnemonic = bip39::generateMnemonic(wc);
        return env->NewStringUTF(mnemonic.c_str());
    } catch (const std::exception& e) {
        LOGE("generateMnemonic exception: %s", e.what());
        return nullptr;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_cryptoscanner_NativeBridge_validateMnemonic(JNIEnv* env, jclass, jstring jMnemonic) {
    try {
        const char* mnemonicChars = env->GetStringUTFChars(jMnemonic, nullptr);
        std::string mnemonic(mnemonicChars);
        env->ReleaseStringUTFChars(jMnemonic, mnemonicChars);
        
        return bip39::validateMnemonic(mnemonic) ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        LOGE("validateMnemonic exception: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jstring JNICALL
Java_com_cryptoscanner_NativeBridge_deriveBitcoinAddress(JNIEnv* env, jclass, jstring jMnemonic, jstring jPassphrase, jint index) {
    try {
        const char* mnemonicChars = env->GetStringUTFChars(jMnemonic, nullptr);
        std::string mnemonic(mnemonicChars);
        env->ReleaseStringUTFChars(jMnemonic, mnemonicChars);

        std::string passphrase = "";
        if (jPassphrase != nullptr) {
            const char* ppChars = env->GetStringUTFChars(jPassphrase, nullptr);
            passphrase = ppChars;
            env->ReleaseStringUTFChars(jPassphrase, ppChars);
        }

        auto seed = bip39::mnemonicToSeed(mnemonic, passphrase);
        auto [priv, pub] = bip32::deriveBIP44(seed, bip32::COIN_BITCOIN, index);
        std::string addr = address::bitcoinP2PKH(pub);

        return env->NewStringUTF(addr.c_str());
    } catch (const std::exception& e) {
        LOGE("deriveBitcoinAddress exception: %s", e.what());
        return nullptr;
    }
}

JNIEXPORT jstring JNICALL
Java_com_cryptoscanner_NativeBridge_deriveBitcoinBech32Address(JNIEnv* env, jclass, jstring jMnemonic, jstring jPassphrase, jint index) {
    try {
        const char* mnemonicChars = env->GetStringUTFChars(jMnemonic, nullptr);
        std::string mnemonic(mnemonicChars);
        env->ReleaseStringUTFChars(jMnemonic, mnemonicChars);

        std::string passphrase = "";
        if (jPassphrase != nullptr) {
            const char* ppChars = env->GetStringUTFChars(jPassphrase, nullptr);
            passphrase = ppChars;
            env->ReleaseStringUTFChars(jPassphrase, ppChars);
        }

        auto seed = bip39::mnemonicToSeed(mnemonic, passphrase);
        auto [priv, pub] = bip32::deriveBIP44(seed, bip32::COIN_BITCOIN, index);
        std::string addr = address::bitcoinBech32(pub);

        return env->NewStringUTF(addr.c_str());
    } catch (const std::exception& e) {
        LOGE("deriveBitcoinBech32Address exception: %s", e.what());
        return nullptr;
    }
}

JNIEXPORT jstring JNICALL
Java_com_cryptoscanner_NativeBridge_deriveEthereumAddress(JNIEnv* env, jclass, jstring jMnemonic, jstring jPassphrase, jint index) {
    try {
        const char* mnemonicChars = env->GetStringUTFChars(jMnemonic, nullptr);
        std::string mnemonic(mnemonicChars);
        env->ReleaseStringUTFChars(jMnemonic, mnemonicChars);

        std::string passphrase = "";
        if (jPassphrase != nullptr) {
            const char* ppChars = env->GetStringUTFChars(jPassphrase, nullptr);
            passphrase = ppChars;
            env->ReleaseStringUTFChars(jPassphrase, ppChars);
        }

        auto seed = bip39::mnemonicToSeed(mnemonic, passphrase);
        
        // Ethereum uses uncompressed pubkey
        auto master = bip32::masterKeyFromSeed(seed);
        auto purpose = bip32::deriveChildPrivate(master, 44 | 0x80000000);
        auto coin = bip32::deriveChildPrivate(purpose, bip32::COIN_ETHEREUM | 0x80000000);
        auto account = bip32::deriveChildPrivate(coin, 0 | 0x80000000);
        auto change = bip32::deriveChildPrivate(account, 0);
        auto child = bip32::deriveChildPrivate(change, index);

        auto uncompressedPub = bip32::getPublicKeyUncompressed(child.key);
        std::string addr = address::ethereumAddress(uncompressedPub);

        return env->NewStringUTF(addr.c_str());
    } catch (const std::exception& e) {
        LOGE("deriveEthereumAddress exception: %s", e.what());
        return nullptr;
    }
}

JNIEXPORT jstring JNICALL
Java_com_cryptoscanner_NativeBridge_deriveSolanaAddress(JNIEnv* env, jclass, jstring jMnemonic, jstring jPassphrase, jint index) {
    try {
        const char* mnemonicChars = env->GetStringUTFChars(jMnemonic, nullptr);
        std::string mnemonic(mnemonicChars);
        env->ReleaseStringUTFChars(jMnemonic, mnemonicChars);

        std::string passphrase = "";
        if (jPassphrase != nullptr) {
            const char* ppChars = env->GetStringUTFChars(jPassphrase, nullptr);
            passphrase = ppChars;
            env->ReleaseStringUTFChars(jPassphrase, ppChars);
        }

        auto seed = bip39::mnemonicToSeed(mnemonic, passphrase);
        auto [priv, pub] = address::deriveSolanaKeypair(seed, index);
        std::string addr = address::solanaAddress(pub);

        return env->NewStringUTF(addr.c_str());
    } catch (const std::exception& e) {
        LOGE("deriveSolanaAddress exception: %s", e.what());
        return nullptr;
    }
}

JNIEXPORT jstring JNICALL
Java_com_cryptoscanner_NativeBridge_deriveAddresses(JNIEnv* env, jclass, jstring jMnemonic, jstring jPassphrase, jint coinType, jint count) {
    try {
        const char* mnemonicChars = env->GetStringUTFChars(jMnemonic, nullptr);
        std::string mnemonic(mnemonicChars);
        env->ReleaseStringUTFChars(jMnemonic, mnemonicChars);

        std::string passphrase = "";
        if (jPassphrase != nullptr) {
            const char* ppChars = env->GetStringUTFChars(jPassphrase, nullptr);
            passphrase = ppChars;
            env->ReleaseStringUTFChars(jPassphrase, ppChars);
        }

        auto seed = bip39::mnemonicToSeed(mnemonic, passphrase);
        json jsonArray = json::array();

        if (coinType == bip32::COIN_SOLANA) {
            for (int i = 0; i < count; ++i) {
                auto [priv, pub] = address::deriveSolanaKeypair(seed, i);
                std::string addr = address::solanaAddress(pub);
                json obj;
                obj["index"] = i;
                obj["address"] = addr;
                obj["privateKeyHex"] = encoding::toHex(priv.data(), priv.size());
                obj["addressType"] = "SOL";
                jsonArray.push_back(obj);
            }
        } else if (coinType == bip32::COIN_ETHEREUM) {
            auto master = bip32::masterKeyFromSeed(seed);
            auto purpose = bip32::deriveChildPrivate(master, 44 | 0x80000000);
            auto coin = bip32::deriveChildPrivate(purpose, bip32::COIN_ETHEREUM | 0x80000000);
            auto account = bip32::deriveChildPrivate(coin, 0 | 0x80000000);
            auto change = bip32::deriveChildPrivate(account, 0);

            for (int i = 0; i < count; ++i) {
                auto child = bip32::deriveChildPrivate(change, i);
                auto uncompressedPub = bip32::getPublicKeyUncompressed(child.key);
                std::string addr = address::ethereumAddress(uncompressedPub);
                json obj;
                obj["index"] = i;
                obj["address"] = addr;
                obj["privateKeyHex"] = encoding::toHex(child.key.data(), child.key.size());
                obj["addressType"] = "ERC20";
                jsonArray.push_back(obj);
            }
        } else if (coinType == bip32::COIN_BITCOIN) {
            auto master = bip32::masterKeyFromSeed(seed);
            auto purpose = bip32::deriveChildPrivate(master, 44 | 0x80000000);
            auto coin = bip32::deriveChildPrivate(purpose, bip32::COIN_BITCOIN | 0x80000000);
            auto account = bip32::deriveChildPrivate(coin, 0 | 0x80000000);
            auto change = bip32::deriveChildPrivate(account, 0);

            for (int i = 0; i < count; ++i) {
                auto child = bip32::deriveChildPrivate(change, i);
                auto compressedPub = bip32::getPublicKeyCompressed(child.key);
                std::string addr1 = address::bitcoinP2PKH(compressedPub);
                std::string addr2 = address::bitcoinBech32(compressedPub);
                
                json obj1;
                obj1["index"] = i;
                obj1["address"] = addr1;
                obj1["privateKeyHex"] = encoding::toHex(child.key.data(), child.key.size());
                obj1["addressType"] = "P2PKH";
                jsonArray.push_back(obj1);

                json obj2;
                obj2["index"] = i;
                obj2["address"] = addr2;
                obj2["privateKeyHex"] = encoding::toHex(child.key.data(), child.key.size());
                obj2["addressType"] = "Bech32";
                jsonArray.push_back(obj2);
            }
        }

        std::string jsonStr = jsonArray.dump();
        return env->NewStringUTF(jsonStr.c_str());

    } catch (const std::exception& e) {
        LOGE("deriveAddresses exception: %s", e.what());
        return nullptr;
    }
}

JNIEXPORT jstring JNICALL
Java_com_cryptoscanner_NativeBridge_getPrivateKeyHex(JNIEnv* env, jclass, jstring jMnemonic, jstring jPassphrase, jint coinType, jint index) {
    try {
        const char* mnemonicChars = env->GetStringUTFChars(jMnemonic, nullptr);
        std::string mnemonic(mnemonicChars);
        env->ReleaseStringUTFChars(jMnemonic, mnemonicChars);

        std::string passphrase = "";
        if (jPassphrase != nullptr) {
            const char* ppChars = env->GetStringUTFChars(jPassphrase, nullptr);
            passphrase = ppChars;
            env->ReleaseStringUTFChars(jPassphrase, ppChars);
        }

        auto seed = bip39::mnemonicToSeed(mnemonic, passphrase);
        
        if (coinType == bip32::COIN_SOLANA) {
            auto [priv, pub] = address::deriveSolanaKeypair(seed, index);
            std::string privHex = encoding::toHex(priv.data(), priv.size());
            return env->NewStringUTF(privHex.c_str());
        } else {
            auto master = bip32::masterKeyFromSeed(seed);
            auto purpose = bip32::deriveChildPrivate(master, 44 | 0x80000000);
            auto coin = bip32::deriveChildPrivate(purpose, coinType | 0x80000000);
            auto account = bip32::deriveChildPrivate(coin, 0 | 0x80000000);
            auto change = bip32::deriveChildPrivate(account, 0);
            auto child = bip32::deriveChildPrivate(change, index);

            std::string privHex = encoding::toHex(child.key.data(), child.key.size());
            return env->NewStringUTF(privHex.c_str());
        }

    } catch (const std::exception& e) {
        LOGE("getPrivateKeyHex exception: %s", e.what());
        return nullptr;
    }
}

} // extern "C"
