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

    struct WorkerConfig {
        uint64_t start_key;
        uint64_t end_key;
        int worker_id;
    };

    class ScannerEngine {
    public:
        ScannerEngine(
            uint64_t lower_bound,
            uint64_t upper_bound,
            const Types::Hash160& target_hash160,
            int num_threads = Config::DEFAULT_WORKER_THREADS
        );
        ~ScannerEngine();

        void start();
        void stop();

    private:
        uint64_t lower_bound_;
        uint64_t upper_bound_;
        Types::Hash160 target_hash160_;
        int num_threads_;

        std::vector<std::thread> workers_;
        std::atomic<bool> running_;
        std::atomic<uint64_t> next_chunk_start_key_;

        ECC::Context ecc_context_;
        Progress::ProgressManager progress_manager_;
        Checkpoint::CheckpointManager checkpoint_manager_;

        std::mutex checkpoint_mutex_;
        std::condition_variable checkpoint_cv_;

        void worker_thread_func(int worker_id);
        void checkpoint_thread_func();

        // Helper to convert uint64_t to PrivateKey (Types::PrivateKey)
        Types::PrivateKey uint64_to_private_key(uint64_t key_value);
        std::string private_key_to_hex(const Types::PrivateKey& priv_key);
        std::string public_key_to_hex(const Types::PublicKeyCompressed& pub_key);
        std::string hash160_to_hex(const Types::Hash160& hash);
        std::string hash160_to_address(const Types::Hash160& hash);
    };

}

#endif // BITCOIN_PUZZLE_SCANNER_ENGINE_H
