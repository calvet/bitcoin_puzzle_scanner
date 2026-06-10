#ifndef BITCOIN_PUZZLE_SCANNER_CHECKPOINT_MANAGER_H
#define BITCOIN_PUZZLE_SCANNER_CHECKPOINT_MANAGER_H

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <filesystem>
#include "progress_manager.h"
#include "types.h"

namespace Checkpoint {

    struct WorkerCheckpointState {
        uint64_t current_private_key_value; // The current private key value for this worker
        // Add other worker-specific states if necessary, e.g., local statistics
    };

    struct GlobalCheckpointState {
        uint64_t lower_bound;
        uint64_t upper_bound;
        uint64_t keys_processed_total;
        uint64_t current_position;
        uint64_t elapsed_seconds;
        std::vector<WorkerCheckpointState> worker_states;
        // Add other global statistics if necessary
    };

    class CheckpointManager {
    public:
        CheckpointManager(const std::string& checkpoint_dir, uint64_t lower_bound, uint64_t upper_bound);

        void save_checkpoint(const Progress::ScanStats& stats, const std::vector<WorkerCheckpointState>& worker_states);
        bool load_latest_checkpoint(Progress::ScanStats& stats, std::vector<WorkerCheckpointState>& worker_states);
        bool checkpoint_exists() const;

    private:
        std::filesystem::path checkpoint_dir_;
        uint64_t initial_lower_bound_;
        uint64_t initial_upper_bound_;

        std::filesystem::path get_checkpoint_file_path() const;
    };

}

#endif // BITCOIN_PUZZLE_SCANNER_CHECKPOINT_MANAGER_H
