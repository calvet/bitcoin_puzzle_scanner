#ifndef BITCOIN_PUZZLE_SCANNER_PROGRESS_MANAGER_H
#define BITCOIN_PUZZLE_SCANNER_PROGRESS_MANAGER_H

#include <cstdint>
#include <string>
#include <chrono>
#include <mutex>
#include "types.h"
#include <mutex>

namespace Progress {

    struct ScanStats {
        Types::UInt256 keys_processed_total;
        Types::UInt256 current_position;
        uint64_t keys_found;
        bool match_found;
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point last_checkpoint_time;
        std::string found_private_key_hex;
        std::string found_public_key_compressed_hex;
        std::string found_address;

        ScanStats() : keys_processed_total(0), current_position(0), keys_found(0), match_found(false) {}
    };

    class ProgressManager {
    public:
        ProgressManager(Types::UInt256 lower_bound, Types::UInt256 upper_bound, int puzzle_number = 0, int num_threads = 1);

        void start_scan();
        void update_progress(uint64_t keys_scanned_in_chunk, Types::UInt256 current_chunk_end_key);
        void report_match(const std::string& priv_key_hex, const std::string& pub_key_compressed_hex, const std::string& address);
        ScanStats get_stats() const;
        void display_dashboard() const;
        bool is_match_found() const { 
            std::lock_guard<std::mutex> lock(stats_mutex_);
            return stats_.match_found; 
        }

    private:
        Types::UInt256 lower_bound_;
        Types::UInt256 upper_bound_;
        Types::UInt256 total_keys_in_interval_;
        int puzzle_number_;
        int num_threads_;
        ScanStats stats_;
        mutable std::mutex stats_mutex_;

        // Helper to calculate keys per second, etc.
        double calculate_keys_per_second() const;
        std::string format_time(long long seconds) const;
    };

}

#endif // BITCOIN_PUZZLE_SCANNER_PROGRESS_MANAGER_H
