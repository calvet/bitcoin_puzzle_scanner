#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

#include "config.h"
#include "types.h"
#include "scanner_engine.h"
#include <iomanip>

int main() {
    std::cout << "Starting Bitcoin Puzzle #" << Config::PUZZLE_NUMBER << " CPU Scanner...\n";
    std::cout << "Target Address: " << Config::TARGET_ADDRESS << "\n";
    std::cout << "Target HASH160: " << Config::TARGET_HASH160_HEX << "\n";
    std::cout << "Search Range: 0x" << Config::LOWER_BOUND << " to 0x" << Config::UPPER_BOUND << "\n";

    Types::Hash160 target_hash160_bytes;
    // Convert hex string to bytes for target_hash160_bytes
    for (size_t i = 0; i < Config::TARGET_HASH160_HEX.length(); i += 2) {
        std::string byteString = Config::TARGET_HASH160_HEX.substr(i, 2);
        target_hash160_bytes[i / 2] = static_cast<uint8_t>(std::stoul(byteString, nullptr, 16));
    }

    try {
        Scanner::ScannerEngine engine(
            Types::UInt256::from_hex(Config::LOWER_BOUND),
            Types::UInt256::from_hex(Config::UPPER_BOUND),
            target_hash160_bytes,
            Config::DEFAULT_WORKER_THREADS
        );
        engine.start();

        // After scan finishes, display final stats if no match was found
        // Or display match details if found (handled by progress manager)

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Scanner finished.\n";

    return 0;
}
