cmake_minimum_required(VERSION 3.15)

project(BitcoinPuzzleScanner LANGUAGES C CXX)

# Default to Release build type if none is specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings and optimizations
if(MSVC)
    add_compile_options(/W4 /arch:AVX2 /wd4996 /O2)
else()
    add_compile_options(-Wall -Wextra -msse4.1 -mavx -mavx2 -O3)
endif()

# Include directories
include_directories(include)

# FetchContent
include(FetchContent)

#
# GoogleTest
#
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(googletest)

#
# libsecp256k1
#
FetchContent_Declare(
    secp256k1
    GIT_REPOSITORY https://github.com/bitcoin-core/secp256k1.git
    GIT_TAG v0.7.0
)

# Configure secp256k1 options BEFORE MakeAvailable
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_EXHAUSTIVE_TESTS OFF CACHE BOOL "" FORCE)

set(SECP256K1_ENABLE_MODULE_ECDH ON CACHE BOOL "" FORCE)
set(SECP256K1_ENABLE_MODULE_EXTRAKEYS ON CACHE BOOL "" FORCE)
set(SECP256K1_ENABLE_MODULE_SCHNORRSIG ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(secp256k1)

#
# Scanner Core
#
add_library(ScannerCore STATIC
    src/sha256.cpp
    src/ripemd160.cpp
    src/sha256_keyhunt.cpp
    src/sha256_sse.cpp
    src/ripemd160_keyhunt.cpp
    src/ripemd160_sse.cpp
    src/ecc.cpp
    src/progress_manager.cpp
    src/checkpoint_manager.cpp
    src/scanner_engine.cpp
    src/util.cpp
    src/secp256k1/Int.cpp
    src/secp256k1/IntGroup.cpp
    src/secp256k1/IntMod.cpp
    src/secp256k1/Point.cpp
    src/secp256k1/Random.cpp
    src/secp256k1/SECP256K1.cpp
)

target_include_directories(ScannerCore
    PUBLIC
        include
        src
)

target_link_libraries(ScannerCore
    PUBLIC
        secp256k1
)

#
# Main executable
#
add_executable(BitcoinPuzzleScanner
    src/main.cpp
)

target_link_libraries(BitcoinPuzzleScanner
    PRIVATE
        ScannerCore
)

#
# Tests
#
enable_testing()

add_executable(scanner_tests
    tests/test_main.cpp
)

target_link_libraries(scanner_tests
    PRIVATE
        gtest_main
        gmock_main
        ScannerCore
)

include(GoogleTest)
gtest_discover_tests(scanner_tests)

#
# Install
#
install(
    TARGETS BitcoinPuzzleScanner
    RUNTIME DESTINATION bin
)

install(
    FILES LICENSE README.md
    DESTINATION .
)