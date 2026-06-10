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
        Types::UInt256 current_private_key_value; // The current private key value for this worker
        // Add other worker-specific states if necessary, e.g., local statistics
    };

    struct GlobalCheckpointState {
        Types::UInt256 lower_bound;
        Types::UInt256 upper_bound;
        Types::UInt256 keys_processed_total;
        Types::UInt256 current_position;
        uint64_t elapsed_seconds;
        std::vector<WorkerCheckpointState> worker_states;
        // Add other global statistics if necessary
    };

    class CheckpointManager {
    public:
        CheckpointManager(const std::string& checkpoint_dir, Types::UInt256 lower_bound, Types::UInt256 upper_bound);

        void save_checkpoint(const Progress::ScanStats& stats, const std::vector<WorkerCheckpointState>& worker_states);
        bool load_latest_checkpoint(Progress::ScanStats& stats, std::vector<WorkerCheckpointState>& worker_states);
        bool checkpoint_exists() const;

    private:
        std::filesystem::path checkpoint_dir_;
        Types::UInt256 initial_lower_bound_;
        Types::UInt256 initial_upper_bound_;

        std::filesystem::path get_checkpoint_file_path() const;
    };

}

#endif // BITCOIN_PUZZLE_SCANNER_CHECKPOINT_MANAGER_H
