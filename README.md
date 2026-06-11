# Bitcoin Puzzle CPU Scanner

## Project Overview

This project is an educational implementation of a high-performance, cross-platform Bitcoin address scanner designed for CPU execution. It is inspired by the architecture and optimization techniques used in tools like KeyHunt, VanitySearch, and BitCrack. The primary goal is to demonstrate the internal workings of Bitcoin address search engines, allowing users to scan for any puzzle from #1 to #160.

## Educational Disclaimer

**This software is intended exclusively for educational and research purposes.** It aims to illustrate the cryptographic principles and computational challenges involved in searching for Bitcoin private keys. It is **not** designed for practical recovery of lost or stolen Bitcoins, nor should it be used for any illicit activities. It operates as a HASH160 address scanner, directly comparing the computed hashes to the target addresses.

## The Bitcoin Challenge Puzzles

In 2015, a series of Bitcoin puzzles were created to demonstrate the vastness of the 256-bit private key space. The creator sent increasing amounts of Bitcoin to 160 different addresses. Each address was generated from a private key confined to a specific, progressively larger range. For example, Puzzle #1's private key was in the range `[1, 2)`, Puzzle #2 in `[2, 4)`, up to Puzzle #160 in `[2^159, 2^160)`. The challenge is to find the private key within the specified range for each address. Many of the lower-tier puzzles have been solved, but higher ones (like Puzzle #71 and above) remain active challenges. You can track the status of all puzzles at [privatekeys.pw/puzzles/bitcoin-puzzle-tx](https://privatekeys.pw/puzzles/bitcoin-puzzle-tx?table=0&status=all).

## Why Pollard Kangaroo Cannot Be Used for These Puzzles

Pollard's Kangaroo algorithm, along with other Elliptic Curve Discrete Logarithm Problem (ECDLP) solving algorithms like Baby-step Giant-step (BSGS), requires knowledge of the public key (P) corresponding to the target private key (k) such that P = k * G (where G is the generator point of the secp256k1 curve). In the case of these Bitcoin Puzzles, only the target Bitcoin address and a range for the private key are publicly known. The corresponding public key is not revealed. Without the public key, ECDLP algorithms are inapplicable, forcing the scanner to perform a brute-force search by generating private keys, deriving their public keys, computing the HASH160, and comparing it against the target HASH160.

## Explanation of HASH160

HASH160 is a cryptographic hash function commonly used in Bitcoin to generate a shorter, fixed-size representation of a public key. It is computed by taking the SHA256 hash of the public key, and then taking the RIPEMD160 hash of the result. The Bitcoin address is then derived from this HASH160 value. In this scanner, instead of converting the HASH160 to a Base58 address for comparison, we directly compare the computed HASH160 of a candidate private key's public key against the target HASH160 of the puzzle's address. This avoids the computational overhead of Base58 encoding and decoding during the scanning process.

## Explanation of Incremental Point Walking

Elliptic Curve Cryptography (ECC) operations, particularly scalar multiplication (k * G), are computationally intensive. In a brute-force search, recomputing k * G for every candidate private key (k) would be highly inefficient. Incremental point walking is an optimization technique that leverages the additive property of elliptic curves. If we have a public key P = k * G, then the next public key in a sequential scan, P' = (k+1) * G, can be efficiently calculated as P' = P + G (elliptic curve point addition). This avoids performing a full scalar multiplication for each increment, significantly speeding up the public key derivation process. The `libsecp256k1` library provides functionalities to perform such incremental additions efficiently.

## Build Instructions

This project uses CMake and requires a C++20 compatible compiler. It is cross-platform and supports Windows, Linux, and macOS.

### Prerequisites
* A C++20 compatible compiler (e.g., MSVC, GCC, Clang)
* CMake (version 3.15 or higher)
* Git

### Building the Project

1. **Clone the repository:**
   ```bash
   git clone https://github.com/calvet/bitcoin_puzzle_scanner.git
   cd bitcoin_puzzle_scanner
   ```

2. **Build with CMake:**

   **On Linux / macOS:**
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   ```

   **On Windows (MSVC):**
   ```bash
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```

This will build the `BitcoinPuzzleScanner` executable and the `scanner_tests` executable.

## Running Instructions

To run the scanner, navigate to the build directory and execute the binary:

**On Linux / macOS:**
```bash
./BitcoinPuzzleScanner
```

**On Windows:**
```cmd
.\Release\BitcoinPuzzleScanner.exe
```

### Interactive Configuration

Upon starting, the scanner will prompt you interactively to configure your session:
1. **Puzzle Number:** Choose a puzzle from 1 to 160 (default is 71).
2. **Threads:** Specify the number of CPU threads to utilize.
3. **Checkpoint Interval:** Set the interval (in seconds) to save your progress.
4. **Scan Mode:** Choose between **Sequential** (linear scan) or **Random** mode.
5. **Random Pause:** Add an optional pause between blocks to prevent rate-limiting or throttle usage.
6. **Verbose Mode:** Optionally enable verbose output to log which block each thread is currently scanning (disabled by default).

When a match is found, the details (Private Key, Public Key, Address) are printed to the console and saved automatically into a `FOUND_PUZZLE_<number>.txt` file.

## Verbose Mode

Verbose mode is an **opt-in** diagnostic feature (disabled by default). When enabled, each worker thread prints a timestamped message every time it starts scanning a new block of keys:

```
[2026-06-11 02:47:00] Worker 0: processing block 0x<start_hex> – 0x<end_hex>
[2026-06-11 02:47:00] Worker 1: processing block 0x<start_hex> – 0x<end_hex>
...
```

Each block covers **100 000 keys**, so the output is informative without being overwhelming. This is useful for verifying that all threads are active, checking for stragglers, and confirming the scanner is covering the expected region of the key-space.

> **Note:** In Random mode the block start addresses will be non-contiguous, which is expected.

## Checkpoint Behavior

The scanner implements a robust checkpoint system. Checkpoints are saved at your configured interval into the `checkpoints/` directory. On startup, the program automatically detects and validates existing checkpoints, resuming the scan from the last saved position for the selected puzzle. 

*(Note: Random mode does not save resumable position checkpoints, only statistical progress).*

## Performance Expectations

This educational implementation focuses on demonstrating optimization techniques rather than achieving peak performance. While it incorporates KeyHunt-inspired optimizations such as incremental point addition, batch processing, and reduced synchronization overhead, its performance will be limited by the CPU-only execution and the overhead of educational logging and error checking. Expect performance in the range of thousands to millions of keys per second, depending on your CPU architecture and clock speed. For practical, high-speed Bitcoin key searching, GPU-accelerated solutions are significantly faster.

## Limitations of CPU Execution

CPU-based brute-force searching for Bitcoin private keys is inherently slow compared to GPU-accelerated methods. The secp256k1 curve operations, while optimized, still require significant computational resources. Modern GPUs can perform these parallel computations orders of magnitude faster due to their architecture being highly suited for such tasks. This project serves as an educational tool to understand the underlying mechanics, not as a practical solution for solving Bitcoin puzzles in a competitive timeframe.

## Future Improvements

Possible future improvements include:

*   **GPU Acceleration:** Integrating CUDA or OpenCL for significantly faster scanning.
*   **Command-line Arguments:** Passing configuration directly via arguments to bypass interactive prompts for headless automation.
*   **GUI:** Developing a graphical user interface for easier interaction and visualization of progress.

## References

*   [Bitcoin Puzzles Challenge Tracker](https://privatekeys.pw/puzzles/bitcoin-puzzle-tx?table=0&status=all)
*   [libsecp256k1 GitHub Repository](https://github.com/bitcoin-core/secp256k1)
*   [SHA-256 Library in C (lucidar.me)](https://lucidar.me/en/dev-c-cpp/sha-256-in-c-cpp/)
*   [cpp-ripemd160 GitHub Repository](https://github.com/miguelmota/cpp-ripemd160)

