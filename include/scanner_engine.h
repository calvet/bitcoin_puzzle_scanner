#ifndef BITCOIN_PUZZLE_SCANNER_ENGINE_H
#define BITCOIN_PUZZLE_SCANNER_ENGINE_H

#include <cstdint>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "config.h"
#include "types.h"
#include "ecc.h"
#include "progress_manager.h"
#include "checkpoint_manager.h"

namespace Scanner {

    enum class ScanMode {
        SEQUENTIAL,
        RANDOM
    };

    struct WorkerConfig {
        Types::UInt256 start_key;
        Types::UInt256 end_key;
        int worker_id;
    };

    class ScannerEngine {
    public:
        ScannerEngine(
            Types::UInt256 lower_bound,
            Types::UInt256 upper_bound,
            const Types::Hash160& target_hash160,
            int num_threads = Config::DEFAULT_WORKER_THREADS,
            int puzzle_number = 0,
            ScanMode mode = ScanMode::SEQUENTIAL,
            int max_pause_seconds = 0
        );
        ~ScannerEngine();

        void start();
        void stop();

    private:
        Types::UInt256 lower_bound_;
        Types::UInt256 upper_bound_;
        Types::Hash160 target_hash160_;
        int num_threads_;
        ScanMode mode_;
        int max_pause_seconds_;
        bool is_compressed_;

        std::vector<std::thread> workers_;
        std::atomic<bool> running_;
        Types::UInt256 next_chunk_start_key_;
        std::mutex chunk_mutex_;

        ECC::Context ecc_context_;
        Progress::ProgressManager progress_manager_;
        Checkpoint::CheckpointManager checkpoint_manager_;

        std::mutex checkpoint_mutex_;
        std::condition_variable checkpoint_cv_;

        void worker_thread_func(int worker_id);
        void checkpoint_thread_func();

        // Helper to convert UInt256 to PrivateKey (Types::PrivateKey)
        Types::PrivateKey uint256_to_private_key(const Types::UInt256& key_value);
        std::string private_key_to_hex(const Types::PrivateKey& priv_key);
        std::string public_key_to_hex(const Types::PublicKeyCompressed& pub_key);
        std::string hash160_to_hex(const Types::Hash160& hash);
    public:
        std::string hash160_to_address(const Types::Hash160& hash);
        Progress::ScanStats get_scan_stats() const { return progress_manager_.get_stats(); }
    };

}

#endif // BITCOIN_PUZZLE_SCANNER_ENGINE_H
