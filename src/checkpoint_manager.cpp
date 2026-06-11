#include "checkpoint_manager.h"
#include <fstream>
#include <iostream>
#include "config.h"

namespace Checkpoint {

    CheckpointManager::CheckpointManager(const std::string& checkpoint_dir, Types::UInt256 lower_bound, Types::UInt256 upper_bound, int puzzle_number)
        : checkpoint_dir_(checkpoint_dir),
          initial_lower_bound_(lower_bound),
          initial_upper_bound_(upper_bound),
          puzzle_number_(puzzle_number) {
        std::filesystem::create_directories(checkpoint_dir_);
    }

    std::filesystem::path CheckpointManager::get_checkpoint_file_path() const {
        return checkpoint_dir_ / ("puzzle_" + std::to_string(puzzle_number_) + ".checkpoint");
    }

    void CheckpointManager::save_checkpoint(const Progress::ScanStats& stats, const std::vector<WorkerCheckpointState>& worker_states) {
        GlobalCheckpointState global_state;
        global_state.puzzle_number = puzzle_number_;
        global_state.lower_bound = initial_lower_bound_;
        global_state.upper_bound = initial_upper_bound_;
        global_state.keys_processed_total = stats.keys_processed_total;
        global_state.current_position = stats.current_position;

        auto now = std::chrono::steady_clock::now();
        global_state.elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - stats.start_time).count();

        global_state.worker_states = worker_states;

        std::ofstream ofs(get_checkpoint_file_path(), std::ios::binary);
        if (!ofs.is_open()) {
            std::cerr << Config::current_time() << "Error: Could not open checkpoint file for writing.\n";
            return;
        }

        ofs.write(reinterpret_cast<const char*>(&global_state.puzzle_number), sizeof(global_state.puzzle_number));
        ofs.write(reinterpret_cast<const char*>(&global_state.lower_bound), sizeof(global_state.lower_bound));
        ofs.write(reinterpret_cast<const char*>(&global_state.upper_bound), sizeof(global_state.upper_bound));
        ofs.write(reinterpret_cast<const char*>(&global_state.keys_processed_total), sizeof(global_state.keys_processed_total));
        ofs.write(reinterpret_cast<const char*>(&global_state.current_position), sizeof(global_state.current_position));
        ofs.write(reinterpret_cast<const char*>(&global_state.elapsed_seconds), sizeof(global_state.elapsed_seconds));

        size_t num_workers = global_state.worker_states.size();
        ofs.write(reinterpret_cast<const char*>(&num_workers), sizeof(num_workers));
        for (const auto& worker_state : global_state.worker_states) {
            ofs.write(reinterpret_cast<const char*>(&worker_state.current_private_key_value), sizeof(worker_state.current_private_key_value));
        }

        ofs.close();
    }

    bool CheckpointManager::load_latest_checkpoint(Progress::ScanStats& stats, std::vector<WorkerCheckpointState>& worker_states) {
        std::ifstream ifs(get_checkpoint_file_path(), std::ios::binary);
        if (!ifs.is_open()) {
            return false;
        }

        GlobalCheckpointState global_state;
        ifs.read(reinterpret_cast<char*>(&global_state.puzzle_number), sizeof(global_state.puzzle_number));
        ifs.read(reinterpret_cast<char*>(&global_state.lower_bound), sizeof(global_state.lower_bound));
        ifs.read(reinterpret_cast<char*>(&global_state.upper_bound), sizeof(global_state.upper_bound));
        ifs.read(reinterpret_cast<char*>(&global_state.keys_processed_total), sizeof(global_state.keys_processed_total));
        ifs.read(reinterpret_cast<char*>(&global_state.current_position), sizeof(global_state.current_position));
        ifs.read(reinterpret_cast<char*>(&global_state.elapsed_seconds), sizeof(global_state.elapsed_seconds));

        size_t num_workers;
        ifs.read(reinterpret_cast<char*>(&num_workers), sizeof(num_workers));
        worker_states.resize(num_workers);
        for (size_t i = 0; i < num_workers; ++i) {
            ifs.read(reinterpret_cast<char*>(&worker_states[i].current_private_key_value), sizeof(worker_states[i].current_private_key_value));
        }

        if (ifs.fail()) {
            std::cerr << Config::current_time() << "Error: Checkpoint file corrupted or incomplete.\n";
            ifs.close();
            return false;
        }

        stats.keys_processed_total = global_state.keys_processed_total;
        stats.current_position = global_state.current_position;
        stats.start_time = std::chrono::steady_clock::now() - std::chrono::seconds(global_state.elapsed_seconds);
        stats.last_checkpoint_time = stats.start_time; // Reset last checkpoint time to start time for now

        ifs.close();
        return true;
    }

    bool CheckpointManager::checkpoint_exists() const {
        return std::filesystem::exists(get_checkpoint_file_path());
    }

    std::vector<GlobalCheckpointState> CheckpointManager::get_all_checkpoints(const std::string& checkpoint_dir) {
        std::vector<GlobalCheckpointState> checkpoints;
        if (!std::filesystem::exists(checkpoint_dir)) return checkpoints;

        for (const auto& entry : std::filesystem::directory_iterator(checkpoint_dir)) {
            if (entry.path().extension() == ".checkpoint") {
                std::ifstream ifs(entry.path(), std::ios::binary);
                if (ifs.is_open()) {
                    GlobalCheckpointState state;
                    ifs.read(reinterpret_cast<char*>(&state.puzzle_number), sizeof(state.puzzle_number));
                    ifs.read(reinterpret_cast<char*>(&state.lower_bound), sizeof(state.lower_bound));
                    ifs.read(reinterpret_cast<char*>(&state.upper_bound), sizeof(state.upper_bound));
                    ifs.read(reinterpret_cast<char*>(&state.keys_processed_total), sizeof(state.keys_processed_total));
                    ifs.read(reinterpret_cast<char*>(&state.current_position), sizeof(state.current_position));
                    ifs.read(reinterpret_cast<char*>(&state.elapsed_seconds), sizeof(state.elapsed_seconds));
                    if (!ifs.fail()) {
                        checkpoints.push_back(state);
                    }
                }
            }
        }
        std::sort(checkpoints.begin(), checkpoints.end(), [](const GlobalCheckpointState& a, const GlobalCheckpointState& b) {
            return a.puzzle_number < b.puzzle_number;
        });
        return checkpoints;
    }

}
