#ifndef BITCOIN_PUZZLE_SCANNER_CONFIG_H
#define BITCOIN_PUZZLE_SCANNER_CONFIG_H

#include <string>
#include <cstdint>

namespace Config {
    // Threading configuration
    const int DEFAULT_WORKER_THREADS = 2; // Default to 2 worker threads

    // Checkpoint configuration
    const int CHECKPOINT_INTERVAL_SECONDS = 60;
    const std::string CHECKPOINT_DIR = "checkpoints";
    const std::string OUTPUT_DIR = "output";
}

#endif // BITCOIN_PUZZLE_SCANNER_CONFIG_H
