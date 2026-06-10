# Bitcoin Puzzle 71 CPU Scanner

## Project Overview

This project is an educational implementation of a Bitcoin address scanner designed specifically for CPU execution on Windows. It is inspired by the architecture and optimization techniques used in tools like KeyHunt, VanitySearch, and BitCrack. The primary goal is to demonstrate the internal workings of Bitcoin address search engines, focusing on the specific challenge of Bitcoin Puzzle #71.

## Educational Disclaimer

**This software is intended exclusively for educational and research purposes.** It aims to illustrate the cryptographic principles and computational challenges involved in searching for Bitcoin private keys. It is **not** designed for practical recovery of lost or stolen Bitcoins, nor should it be used for any illicit activities. The target configuration is preconfigured for Bitcoin Puzzle #71, which publicly exposes only the Bitcoin address and its corresponding search interval. The public key is unknown, meaning algorithms like Pollard Kangaroo, BSGS, and other ECDLP methods cannot be used. Therefore, this software operates as a HASH160 address scanner.

## Why Pollard Kangaroo Cannot Be Used for Puzzle 71

Pollard's Kangaroo algorithm, along with other Elliptic Curve Discrete Logarithm Problem (ECDLP) solving algorithms like Baby-step Giant-step (BSGS), requires knowledge of the public key (P) corresponding to the target private key (k) such that P = k * G (where G is the generator point of the secp256k1 curve). In the case of Bitcoin Puzzle #71, only the target Bitcoin address and a range for the private key are publicly known. The corresponding public key is not revealed. Without the public key, ECDLP algorithms are inapplicable, forcing the scanner to perform a brute-force search by generating private keys, deriving their public keys, computing the HASH160, and comparing it against the target HASH160.

## Explanation of HASH160

HASH160 is a cryptographic hash function commonly used in Bitcoin to generate a shorter, fixed-size representation of a public key. It is computed by taking the SHA256 hash of the public key, and then taking the RIPEMD160 hash of the result. The Bitcoin address is then derived from this HASH160 value. In this scanner, instead of converting the HASH160 to a Base58 address for comparison, we directly compare the computed HASH160 of a candidate private key's public key against the target HASH160 of Puzzle #71's address. This avoids the computational overhead of Base58 encoding and decoding during the scanning process.

## Explanation of Incremental Point Walking

Elliptic Curve Cryptography (ECC) operations, particularly scalar multiplication (k * G), are computationally intensive. In a brute-force search, recomputing k * G for every candidate private key (k) would be highly inefficient. Incremental point walking is an optimization technique that leverages the additive property of elliptic curves. If we have a public key P = k * G, then the next public key in a sequential scan, P' = (k+1) * G, can be efficiently calculated as P' = P + G (elliptic curve point addition). This avoids performing a full scalar multiplication for each increment, significantly speeding up the public key derivation process. The `libsecp256k1` library provides functionalities to perform such incremental additions efficiently.

## Build Instructions

This project uses CMake as its build system. To build the project, follow these steps:

1.  **Prerequisites:**
    *   A C++20 compatible compiler (MSVC recommended for Windows).
    *   CMake (version 3.15 or higher).
    *   Git.

2.  **Clone the repository:**
    ```bash
    git clone https://github.com/your-username/BitcoinPuzzleScanner.git
    cd BitcoinPuzzleScanner
    ```

3.  **Build with CMake:**
    ```bash
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build .
    ```

    This will build the `BitcoinPuzzleScanner` executable and the `run_tests` executable in the `build/Release` directory.

## Running Instructions

To run the scanner, navigate to the build directory and execute the `BitcoinPuzzleScanner.exe`:

```bash
cd build\Release
.\BitcoinPuzzleScanner.exe
```

The program is preconfigured for Bitcoin Puzzle #71 and will start scanning immediately without requiring command-line arguments.

## Checkpoint Behavior

The scanner implements a robust checkpoint system to save progress periodically and resume interrupted searches. Checkpoints are saved every 60 seconds into the `/checkpoints` directory. On startup, the program automatically detects and validates existing checkpoints, resuming the scan from the last saved position. This ensures that progress is not lost due to unexpected interruptions.

## Performance Expectations

This educational implementation focuses on demonstrating optimization techniques rather than achieving peak performance. While it incorporates KeyHunt-inspired optimizations such as incremental point addition, batch processing, and reduced synchronization overhead, its performance will be limited by the CPU-only execution and the overhead of educational logging and error checking. Expect performance in the range of thousands to millions of keys per second, depending on your CPU architecture and clock speed. For practical, high-speed Bitcoin key searching, GPU-accelerated solutions are significantly faster.

## Limitations of CPU Execution

CPU-based brute-force searching for Bitcoin private keys is inherently slow compared to GPU-accelerated methods. The secp256k1 curve operations, while optimized, still require significant computational resources. Modern GPUs can perform these parallel computations orders of magnitude faster due to their architecture being highly suited for such tasks. This project serves as an educational tool to understand the underlying mechanics, not as a practical solution for solving Bitcoin puzzles in a competitive timeframe.

## Future Improvements

Possible future improvements include:

*   **GPU Acceleration:** Integrating CUDA or OpenCL for significantly faster scanning.
*   **Command-line Arguments:** Allowing users to specify puzzle numbers, search ranges, and thread counts via command-line arguments.
*   **GUI:** Developing a graphical user interface for easier interaction and visualization of progress.
*   **More Robust Checkpointing:** Implementing more sophisticated checkpointing with redundancy and integrity checks.
*   **Cross-platform Compatibility:** Extending support to Linux and macOS.

## References

*   [Bitcoin Puzzle #71 Target Address](https://privatekeys.pw/address/bitcoin/1PWo3JeB9jrGwfHDNpdGK54CRas7fsVzXU)
*   [libsecp256k1 GitHub Repository](https://github.com/bitcoin-core/secp256k1)
*   [SHA-256 Library in C (lucidar.me)](https://lucidar.me/en/dev-c-cpp/sha-256-in-c-cpp/)
*   [cpp-ripemd160 GitHub Repository](https://github.com/miguelmota/cpp-ripemd160)

