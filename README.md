# Bitcoin Puzzle CPU Scanner (AI-Powered)

## Project Overview

This project is a high-performance, cross-platform Bitcoin address scanner designed for CPU execution. It operates as a HASH160 address scanner, directly comparing the computed hashes to target addresses, allowing users to scan for any puzzle from #1 to #160 of the famous Bitcoin Challenge.

> **100% AI Generated:** This entire project—from the core cryptographic algorithms to the build system and optimizations—was built **100% using AI** (specifically using **Gemini 3.1 Pro** and its underlying coding models).

> **Validated Accuracy:** The scanner has been thoroughly tested. Puzzles **#1 through #35** have been run and **all of them validated successfully!**

## Why is it so fast?

You might notice this scanner performing at **6.5M to 7.0M keys/s**, whereas older compiled versions of Keyhunt (e.g., v19.05.23) might cap around 3M keys/s on the same hardware. The massive performance leap comes from several modern optimizations:

1. **AVX/AVX2 and SSE4.1 Acceleration:** The core cryptographic hashing functions (SHA-256 and RIPEMD-160) have been heavily optimized using SIMD instructions, allowing the CPU to hash multiple keys simultaneously.
2. **Incremental Point Walking:** Instead of doing a full elliptic curve scalar multiplication (which is very expensive) for every single key, we use the `libsecp256k1` library to perform highly efficient elliptic curve point additions ($P' = P + G$).
3. **Batch Processing Architecture:** Computations are grouped into blocks (100,000 keys per block). This drastically minimizes thread synchronization overhead, avoids lock contention, and maximizes CPU cache hits.
4. **Modern C++20 Compiler Optimizations:** Leveraging the latest compiler flags (`-O3`, `-mavx2`, `-msse4.1`) and modern memory management leads to a much leaner and faster binary compared to older legacy C/C++ builds.

## Features, Parameters & Options

Upon starting the program, it prompts you interactively to configure your session. Available parameters and options include:

*   **Puzzle Number:** Choose any puzzle from 1 to 160.
*   **Threads:** Specify the number of CPU threads to utilize. Maximize this for highest performance.
*   **Checkpoint Interval:** Set the interval (in seconds) to save your progress automatically.
*   **Scan Mode:**
    *   **Sequential:** Performs a linear, exhaustive scan of the private key range. Best for lower puzzles.
    *   **Random:** Randomly jumps around the key space. Useful for attempting lucky strikes on impossibly large ranges.
*   **Random Pause:** Add an optional pause between blocks to prevent rate-limiting or CPU thermal throttling if you plan to run it 24/7.
*   **Verbose Mode:** An optional diagnostic feature. When enabled, each worker thread prints a timestamped message every time it starts scanning a new block of keys.
*   **Telegram Notifications:** Input your Telegram Bot Token and Chat ID. When a puzzle is found, the scanner will immediately send a secure notification to your phone.

When a match is found, the private key, public key, and address are printed to the console and automatically saved to a file named `FOUND_PUZZLE_<number>.txt`.

## Build Instructions

### Prerequisites
*   A C++20 compatible compiler (MSVC, GCC, Clang)
*   CMake (version 3.15 or higher)
*   Git

### Building

**On Linux / macOS:**
```bash
git clone https://github.com/calvet/bitcoin_puzzle_scanner.git
cd bitcoin_puzzle_scanner
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**On Windows (MSVC):**
```cmd
git clone https://github.com/calvet/bitcoin_puzzle_scanner.git
cd bitcoin_puzzle_scanner
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Running Instructions

Navigate to your build directory and execute the binary:

**Linux / macOS:**
```bash
./BitcoinPuzzleScanner
```

**Windows:**
```cmd
.\Release\BitcoinPuzzleScanner.exe
```

## Checkpoint System

The scanner features a robust, automatic checkpoint system. It saves your progress into the `checkpoints/` directory at the interval you specify. If you stop the program or your computer reboots, simply start it again and select the same puzzle. It will automatically detect the checkpoint and resume exactly from where it left off! *(Note: Sequential mode only).*

## Support / Donate

If you found this tool useful, learned from the source code, or managed to find a high-value puzzle and want to show some appreciation, donations are very welcome!

**My Bitcoin Address:** `1NuGAFYjjg17JjPNCXFZ8HCZQMNWiN9bmy`
