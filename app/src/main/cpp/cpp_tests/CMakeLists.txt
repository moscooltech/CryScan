cmake_minimum_required(VERSION 3.22)
project(CryptoScannerTests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

# The library under test (desktop build, without Android-specific parts)
add_library(cryptoscanner_test_lib STATIC
    ../bip39/bip39.cpp
    ../bip32/bip32.cpp
    ../crypto/sha256.cpp
    ../crypto/sha512.cpp
    ../crypto/hmac.cpp
    ../crypto/ripemd160.cpp
    ../crypto/keccak256.cpp
    ../encoding/hex.cpp
    ../encoding/base58.cpp
    ../encoding/bech32.cpp
    ../address/bitcoin_address.cpp
    ../address/ethereum_address.cpp
    ../address/solana_address.cpp
)
target_include_directories(cryptoscanner_test_lib PUBLIC ..)

# Tests executable
add_executable(cryptoscanner_tests
    bip39_test.cpp
    bip32_test.cpp
    address_test.cpp
    crypto_test.cpp
)
target_link_libraries(cryptoscanner_tests
    cryptoscanner_test_lib
    gtest_main
    gmock_main
)

include(GoogleTest)
gtest_discover_tests(cryptoscanner_tests)
