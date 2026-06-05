cmake_minimum_required(VERSION 3.22)
project(cryptoscanner VERSION 1.0.0 LANGUAGES C CXX)

# Ensure the C compiler is enabled for Android NDK cross-compiles (prevents missing
# internal variables like CMAKE_C_COMPILE_OBJECT when C support was omitted)
enable_language(C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── FetchContent Setup ────────────────────────────────────────────────────────
include(FetchContent)

# libsecp256k1 (Bitcoin Core)
FetchContent_Declare(
    secp256k1
    GIT_REPOSITORY https://github.com/bitcoin-core/secp256k1.git
    GIT_TAG        v0.4.1
    GIT_SHALLOW    TRUE
)
set(SECP256K1_BUILD_EXHAUSTIVE_TESTS OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
set(SECP256K1_ENABLE_MODULE_RECOVERY ON CACHE BOOL "" FORCE)
set(SECP256K1_ENABLE_MODULE_EXTRAKEYS ON CACHE BOOL "" FORCE)
set(SECP256K1_ENABLE_MODULE_SCHNORRSIG ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(secp256k1)

# nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# ─── Source files ──────────────────────────────────────────────────────────────
file(GLOB CRYPTO_SOURCES   "${CMAKE_CURRENT_SOURCE_DIR}/crypto/*.cpp")
file(GLOB ENCODING_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/encoding/*.cpp")
file(GLOB BIP39_SOURCES    "${CMAKE_CURRENT_SOURCE_DIR}/bip39/*.cpp")
file(GLOB BIP32_SOURCES    "${CMAKE_CURRENT_SOURCE_DIR}/bip32/*.cpp")
file(GLOB ADDRESS_SOURCES  "${CMAKE_CURRENT_SOURCE_DIR}/address/*.cpp")
file(GLOB JNI_SOURCES      "${CMAKE_CURRENT_SOURCE_DIR}/jni/*.cpp")
file(GLOB ED25519_SOURCES  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ed25519/src/*.c")

set(ALL_SOURCES
    ${CRYPTO_SOURCES}
    ${ENCODING_SOURCES}
    ${BIP39_SOURCES}
    ${BIP32_SOURCES}
    ${ADDRESS_SOURCES}
    ${JNI_SOURCES}
    ${ED25519_SOURCES}
)

# ─── Shared Library (loaded by Android) ───────────────────────────────────────
add_library(cryptoscanner SHARED ${ALL_SOURCES})

target_include_directories(cryptoscanner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto
    ${CMAKE_CURRENT_SOURCE_DIR}/encoding
    ${CMAKE_CURRENT_SOURCE_DIR}/bip39
    ${CMAKE_CURRENT_SOURCE_DIR}/bip32
    ${CMAKE_CURRENT_SOURCE_DIR}/address
    ${CMAKE_CURRENT_SOURCE_DIR}/jni
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ed25519/src
    ${secp256k1_SOURCE_DIR}/include
)

target_compile_options(cryptoscanner PRIVATE
    -Wall
    -Wextra
    -Werror=return-type
    -fvisibility=hidden
    -fstack-protector-strong
    -O2
)

# Android log library
find_library(log-lib log)

target_link_libraries(cryptoscanner
    PRIVATE
        secp256k1
        nlohmann_json::nlohmann_json
        ${log-lib}
)

# ─── Unit Test Executable (cpp_tests/) ────────────────────────────────────────
# NOTE: C++ tests are disabled for Android mobile builds. Tests can be built and run
# separately on desktop environments. Enable this section if needed for desktop builds.
#
# FetchContent_Declare(
#     googletest
#     GIT_REPOSITORY https://github.com/google/googletest.git
#     GIT_TAG        v1.14.0
#     GIT_SHALLOW    TRUE
# )
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)
#
# file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/cpp_tests/*.cpp")
#
# if(TEST_SOURCES)
#     add_executable(cpp_tests
#         ${TEST_SOURCES}
#         ${CRYPTO_SOURCES}
#         ${ENCODING_SOURCES}
#         ${BIP39_SOURCES}
#         ${BIP32_SOURCES}
#         ${ADDRESS_SOURCES}
#         ${ED25519_SOURCES}
#     )
#
#     target_include_directories(cpp_tests PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}/crypto
#         ${CMAKE_CURRENT_SOURCE_DIR}/encoding
#         ${CMAKE_CURRENT_SOURCE_DIR}/bip39
#         ${CMAKE_CURRENT_SOURCE_DIR}/bip32
#         ${CMAKE_CURRENT_SOURCE_DIR}/address
#         ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ed25519/src
#         ${secp256k1_SOURCE_DIR}/include
#     )
#
#     target_link_libraries(cpp_tests
#         PRIVATE
#             secp256k1
#             nlohmann_json::nlohmann_json
#             GTest::gtest_main
#             ${log-lib}
#     )
#
#     include(GoogleTest)
#     gtest_discover_tests(cpp_tests)
# endif()
