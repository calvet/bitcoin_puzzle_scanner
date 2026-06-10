#ifndef BITCOIN_PUZZLE_SCANNER_PROGRESS_MANAGER_H
#define BITCOIN_PUZZLE_SCANNER_PROGRESS_MANAGER_H

#include <cstdint>
#include <string>
#include <atomic>
#include <chrono>
#include <mutex>

namespace Progress {

    struct ScanStats {
        std::atomic<uint64_t> keys_processed_total;
        std::atomic<uint64_t> current_position;
        std::atomic<uint64_t> keys_found;
        std::atomic<bool> match_found;
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point last_checkpoint_time;
        std::string found_private_key_hex;
        std::string found_public_key_compressed_hex;
        std::string found_address;

        ScanStats() : keys_processed_total(0), current_position(0), keys_found(0), match_found(false) {}
    };

    class ProgressManager {
    public:
        ProgressManager(uint64_t lower_bound, uint64_t upper_bound);

        void start_scan();
        void update_progress(uint64_t keys_scanned_in_chunk, uint64_t current_chunk_end_key);
        void report_match(const std::string& priv_key_hex, const std::string& pub_key_compressed_hex, const std::string& address);
        ScanStats get_stats() const;
        void display_dashboard() const;
        bool is_match_found() const { return stats_.match_found.load(); }

    private:
        uint64_t lower_bound_;
        uint64_t upper_bound_;
        uint64_t total_keys_in_interval_;
        ScanStats stats_;
        mutable std::mutex stats_mutex_;

        // Helper to calculate keys per second, etc.
        double calculate_keys_per_second() const;
        std::string format_time(long long seconds) const;
    };

}

#endif // BITCOIN_PUZZLE_SCANNER_PROGRESS_MANAGER_H
