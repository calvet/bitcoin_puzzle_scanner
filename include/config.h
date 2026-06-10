#ifndef BITCOIN_PUZZLE_SCANNER_CONFIG_H
#define BITCOIN_PUZZIN_PUZZLE_SCANNER_CONFIG_H

#include <string>
#include <cstdint>

namespace Config {
    // Bitcoin Puzzle #71 Configuration
    const int PUZZLE_NUMBER = 71;
    const std::string TARGET_ADDRESS = "1PWo3JeB9jrGwfHDNpdGK54CRas7fsVzXU";
    const std::string TARGET_HASH160_HEX = "f6f5431d25bbf7b12e8add9af5e3475c44a0a5b8";

    // Private key search range for Puzzle #71
    // Lower Bound: 0x400000000000000000
    // Upper Bound: 0x7FFFFFFFFFFFFFFFFF
    // Represented as 64-bit unsigned integers (uint64_t) for simplicity in C++
    // Note: Bitcoin private keys are 256-bit, but for this puzzle, the range is within 64-bit for demonstration.
    // A full implementation would use a bignum library for 256-bit integers.
    const uint64_t LOWER_BOUND = 0x4000000000000000ULL;
    const uint64_t UPPER_BOUND = 0x7FFFFFFFFFFFFFFFFFULL;

    // Threading configuration
    const int DEFAULT_WORKER_THREADS = 2; // Default to 2 worker threads

    // Checkpoint configuration
    const int CHECKPOINT_INTERVAL_SECONDS = 60;
    const std::string CHECKPOINT_DIR = "checkpoints";
    const std::string OUTPUT_DIR = "output";
}

#endif // BITCOIN_PUZZLE_SCANNER_CONFIG_H
