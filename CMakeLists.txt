cmake_minimum_required(VERSION 3.15)
project(BitcoinPuzzleScanner CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable compiler warnings and treat them as errors
if (MSVC)
    add_compile_options(/W4 /WX)
else ()
    add_compile_options(-Wall -Wextra -Werror)
endif ()

# Include directories
include_directories(include)

# FetchContent for dependencies
include(FetchContent)

# Fetch GoogleTest
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
FetchContent_MakeAvailable(googletest)

# Fetch libsecp256k1
FetchContent_Declare(
    secp256k1
    GIT_REPOSITORY https://github.com/bitcoin-core/secp256k1.git
    GIT_TAG master # Or a specific commit/tag
)
FetchContent_MakeAvailable(secp256k1)

# Fetch OpenSSL (or equivalent for SHA256 and RIPEMD160)
# For simplicity, we'll assume a system-wide OpenSSL or provide a basic implementation for now.
# A full FetchContent for OpenSSL is complex due to its build system.
# For this educational project, we might start with a simpler approach or a minimal SHA256/RIPEMD160 implementation.
# If a full OpenSSL dependency is strictly required via FetchContent, it would need more advanced CMake.
# For now, we'll add a placeholder and assume SHA256/RIPEMD160 will be handled either by secp256k1's internal utils or a simple implementation.

# Add library for core logic
add_library(ScannerCore STATIC
    src/sha256.cpp
    src/ripemd160.cpp
    src/ecc.cpp
    src/progress_manager.cpp
    src/checkpoint_manager.cpp
    src/scanner_engine.cpp
)
target_link_libraries(ScannerCore PRIVATE secp256k1::secp256k1)

# Add executable
add_executable(BitcoinPuzzleScanner src/main.cpp)
target_link_libraries(BitcoinPuzzleScanner PRIVATE ScannerCore)

# Add tests
add_executable(run_tests tests/test_main.cpp)
target_link_libraries(run_tests PRIVATE gtest_main gmock_main ScannerCore)

# Install rules (optional for this project, but good practice)
install(TARGETS BitcoinPuzzleScanner DESTINATION bin)
install(FILES LICENSE README.md DESTINATION .)
