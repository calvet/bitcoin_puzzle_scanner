#ifndef BITCOIN_PUZZLE_SCANNER_CONFIG_H
#define BITCOIN_PUZZLE_SCANNER_CONFIG_H

#include <string>
#include <cstdint>
#include <iomanip>
#include <ctime>
#include <sstream>

namespace Config {
    inline std::string current_time() {
        std::time_t t = std::time(nullptr);
        std::tm* tm = std::localtime(&t);
        std::stringstream ss;
        ss << "[" << std::put_time(tm, "%Y-%m-%d %H:%M:%S") << "] ";
        return ss.str();
    }
    // Threading configuration
    const int DEFAULT_WORKER_THREADS = 2; // Default to 2 worker threads

    // Checkpoint configuration
    inline int CHECKPOINT_INTERVAL_SECONDS = 15;
    const std::string CHECKPOINT_DIR = "checkpoints";
    const std::string OUTPUT_DIR = "output";
}

#endif // BITCOIN_PUZZLE_SCANNER_CONFIG_H
