# CryptoScanner 🔍

A high-performance Android application for BIP-39/BIP-32/BIP-44 HD wallet discovery, written in **C++ via the Android NDK** with a **Kotlin** UI layer.

> ⚠️ **EDUCATIONAL USE ONLY.** The probability of finding a funded wallet by random mnemonic generation is astronomically small (≈ 1 in 2¹²⁸). This tool is intended for security research, cryptocurrency education, and personal recovery assistance only. Do not use for malicious purposes.

---

## ✨ Features

- **BIP-39 Mnemonic Generation** — Cryptographically secure 12/15/18/21/24-word phrases
- **HD Wallet Derivation (BIP-32/44)** — Full hierarchical deterministic key derivation
- **Multi-Network Support:**
  - **Bitcoin (BTC)** — P2PKH (legacy) + Bech32 SegWit addresses
  - **Ethereum (ETH)** — EIP-55 checksummed addresses
  - **Solana (SOL)** — Ed25519-based addresses
  - **EVM Chains** — Polygon, BNB Chain, Avalanche (same derivation as ETH)
- **Balance Checking** — Public free APIs with automatic rotation and rate-limit backoff
- **Encrypted Storage** — Android Keystore + SQLCipher Room database
- **Real-Time UI** — Live log console showing derivation and checking progress
- **CI/CD** — GitHub Actions pipeline building APKs for all 4 Android ABIs

---

## 📐 Architecture

```
┌─────────────────────────────────────┐
│           Kotlin UI Layer           │
│  MainActivity → ScanFragment        │
│             → ResultsFragment       │
├─────────────────────────────────────┤
│         ScannerViewModel            │
│         WalletRepository            │
├──────────────┬──────────────────────┤
│  NativeBridge│  Network API Layer   │
│  (JNI)       │  BitcoinApi          │
│              │  EthereumApi         │
│              │  SolanaApi           │
│              │  ApiRotator          │
├──────────────┤──────────────────────┤
│  C++ NDK Core│  Storage Layer       │
│  bip39/      │  KeystoreManager     │
│  bip32/      │  WalletDatabase      │
│  crypto/     │  (Room + SQLCipher)  │
│  encoding/   │                      │
│  address/    │                      │
└──────────────┴──────────────────────┘
```

### C++ Modules (NDK)
| Module | Description |
|--------|-------------|
| `crypto/sha256` | Pure C++ SHA-256 (FIPS 180-4) |
| `crypto/sha512` | Pure C++ SHA-512 (FIPS 180-4) |
| `crypto/hmac` | HMAC-SHA256/512 + PBKDF2-HMAC-SHA512 |
| `crypto/ripemd160` | RIPEMD-160 for Bitcoin hash160 |
| `crypto/keccak256` | Keccak-256 (Ethereum, NOT SHA3-256) |
| `encoding/base58` | Base58 + Base58Check |
| `encoding/bech32` | Bech32 SegWit encoding (BIP-173) |
| `encoding/hex` | Hex encode/decode utilities |
| `bip39/` | BIP-39 mnemonic generation & validation |
| `bip32/` | BIP-32/44 HD key derivation via libsecp256k1 |
| `address/bitcoin` | P2PKH + Bech32 address generation |
| `address/ethereum` | EIP-55 checksummed address generation |
| `address/solana` | SLIP-0010 Ed25519 + Base58 address |
| `jni/` | JNI bridge exposing all C++ to Kotlin |

### Network API Rotation
Each cryptocurrency uses 3–4 free public APIs with automatic failover:

**Bitcoin:**
1. `blockchain.info` (primary)
2. `mempool.space` (secondary)
3. `blockstream.info` (tertiary)

**Ethereum/EVM:**
1. `cloudflare-eth.com` (primary)
2. `rpc.ankr.com/eth` (secondary)
3. `ethereum.publicnode.com` (tertiary)
4. `eth.llamarpc.com` (quaternary)

**Solana:**
1. `api.mainnet-beta.solana.com` (primary)
2. `rpc.ankr.com/solana` (secondary)
3. `solana-api.projectserum.com` (tertiary)

---

## 🏗️ Prerequisites

### Local Development
- **Android Studio** Hedgehog (2023.1.1) or later
- **Android NDK** 26.3.11579264 (install via SDK Manager)
- **CMake** 3.22.1+ (install via SDK Manager)
- **JDK 17**
- **Git** (for FetchContent dependencies: libsecp256k1, nlohmann/json)

### GitHub Actions (CI/CD)
- GitHub repository with Actions enabled
- For production-signed releases: optional GitHub Secrets (see below)

---

## 🚀 Getting Started

### 1. Clone the Repository

```bash
git clone https://github.com/YOUR_USERNAME/CryptoScanner.git
cd CryptoScanner
```

### 2. Open in Android Studio

1. Open Android Studio → **File → Open** → select the `CryptoScanner` folder
2. Wait for Gradle sync to complete
3. Android Studio will automatically download NDK/CMake if prompted

### 3. Configure API Keys (Optional)

The app uses free public APIs that require no keys by default. If you want dedicated keys for higher rate limits, create `local.properties`:

```properties
# Optional — increases rate limits
BLOCKCYPHER_TOKEN=your_token_here
INFURA_PROJECT_ID=your_project_id_here
```

Without these, the app falls back to fully public endpoints.

### 4. Build & Run

```bash
# Debug build (auto-signed, immediately installable)
./gradlew assembleDebug

# Install on connected device/emulator
./gradlew installDebug
```

---

## 📱 Using the App

### Scan Tab
1. **Select Networks** — Tap chips to enable/disable: BTC, ETH, SOL, MATIC, BNB, AVAX
2. **Word Count** — Select mnemonic length (12 words recommended)
3. **Passphrase** — Optional BIP-39 passphrase (leave empty for standard wallets)
4. **Custom Mnemonic** — Expand section to enter an existing mnemonic to check
5. **START** — Begins continuous generation and scanning
6. **STOP** — Halts the scanning loop

The log console shows real-time progress. Found wallets appear in the **Results** tab.

### Results Tab
- Lists all discovered funded wallets
- Long-press an address to copy
- Export button copies all results as JSON
- Clear button removes all stored results (with confirmation)

---

## 🏗️ Building for Production

### Debug APK (default, no signing setup needed)
```bash
./gradlew assembleDebug
# Output: app/build/outputs/apk/debug/app-debug.apk
```

### Release APK with CI-Generated Keystore
The CI/CD pipeline automatically generates a keystore on-the-fly for releases.
**No secrets needed** — just push a version tag:

```bash
git tag v1.0.0
git push origin v1.0.0
```

The GitHub Actions workflow will build, sign, and publish the release APK automatically.

### Release APK with Your Own Keystore (Recommended for Distribution)

1. **Generate a keystore:**
   ```bash
   keytool -genkeypair \
     -alias cryptoscanner-release \
     -keyalg RSA -keysize 4096 -validity 10000 \
     -keystore keystore/release.jks \
     -storepass YOUR_STORE_PASSWORD \
     -keypass YOUR_KEY_PASSWORD \
     -dname "CN=Your Name, O=Your Org, C=US"
   ```

2. **Base64 encode it:**
   ```bash
   base64 -w 0 keystore/release.jks > keystore/release.jks.b64
   ```

3. **Add GitHub Secrets** (Settings → Secrets → Actions):
   | Secret | Value |
   |--------|-------|
   | `RELEASE_KEYSTORE_BASE64` | Contents of `release.jks.b64` |
   | `KEYSTORE_PASSWORD` | Your store password |
   | `KEY_ALIAS` | `cryptoscanner-release` |
   | `KEY_PASSWORD` | Your key password |

4. Push a tag → the workflow automatically uses your keystore.

> ⚠️ **Never commit `keystore/release.jks` to git.** It's in `.gitignore`.

---

## 🧪 Testing

### C++ Unit Tests
```bash
# Run via CTest (requires CMake build)
cd app/src/main/cpp
mkdir -p build && cd build
cmake .. -DANDROID_ABI=x86_64 -DBUILD_TESTS=ON
make && ctest --output-on-failure
```

Tests use known BIP-39/BIP-32/BIP-44 test vectors from the official specifications.

### Android Unit Tests
```bash
./gradlew testDebugUnitTest
# Report: app/build/reports/tests/testDebugUnitTest/index.html
```

### Android Instrumentation Tests
```bash
./gradlew connectedAndroidTest
# Requires connected device or running emulator
```

---

## 📦 CI/CD Pipeline

The GitHub Actions workflow (`.github/workflows/android-ci.yml`) runs on every push and PR:

| Job | Trigger | What it does |
|-----|---------|--------------|
| `build-and-test` | Every push/PR | Lint, unit tests, debug APK |
| `build-multi-abi` | After build-and-test | Validates each ABI separately |
| `release` | Tag push `v*` | Signed release APK + GitHub Release |
| `dependency-check` | Push to main | Security/update scan |

### ABIs Built
- `arm64-v8a` — Modern Android phones (Pixel 3+, Galaxy S10+)
- `armeabi-v7a` — Older Android devices
- `x86_64` — Android emulators (64-bit)
- `x86` — Android emulators (32-bit)

---

## 🔒 Security

### Private Key Protection
- Private keys are **never stored in plaintext**
- All sensitive data encrypted with AES-256-GCM
- Encryption keys managed by **Android Keystore System** (hardware-backed on supported devices)
- Database encrypted with **SQLCipher** using a key derived from Keystore

### Cryptographic Libraries
- **libsecp256k1** (bitcoin-core) — secp256k1 elliptic curve operations (BTC/ETH)
- **Ed25519** — Twisted Edwards curve operations (SOL)
- Pure C++ implementations for hash functions (SHA-256/512, RIPEMD-160, Keccak-256)

### Network Security
- HTTPS only (enforced via `network_security_config.xml`)
- No cleartext traffic permitted
- Certificate validation not disabled

---

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/new-network`
3. Commit changes: `git commit -m 'Add Tron network support'`
4. Push: `git push origin feature/new-network`
5. Open a Pull Request

### Adding a New Network
1. Add coin type constant in `bip32/bip32.h`
2. Implement address generation in `address/` (new .h/.cpp files)
3. Add JNI bridge function in `jni/jni_bridge.cpp`
4. Add Kotlin external declaration in `NativeBridge.kt`
5. Create network API class in `network/`
6. Add `CryptoNetwork` enum entry in `model/ScanConfig.kt`
7. Update `WalletRepository.kt` to handle new network
8. Add UI chip in `fragment_scan.xml`

---

## 📄 License

This project is licensed under the MIT License — see [LICENSE](LICENSE) for details.

---

## 🙏 Acknowledgments

- [BIP-39 Specification](https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki) — Mnemonic code for generating deterministic keys
- [BIP-32 Specification](https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki) — Hierarchical Deterministic Wallets
- [BIP-44 Specification](https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki) — Multi-Account Hierarchy for Deterministic Wallets
- [SLIP-0010](https://github.com/satoshilabs/slips/blob/master/slip-0010.md) — Universal private key derivation from master private key (for Ed25519/Solana)
- [libsecp256k1](https://github.com/bitcoin-core/secp256k1) — Bitcoin's optimized elliptic curve library
- [nlohmann/json](https://github.com/nlohmann/json) — JSON for Modern C++
