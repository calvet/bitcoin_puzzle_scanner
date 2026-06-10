#include "progress_manager.h"
#include "config.h"
#include <iostream>
#include <iomanip>
#include <cmath>

namespace Progress {

    ProgressManager::ProgressManager(uint64_t lower_bound, uint64_t upper_bound)
        : lower_bound_(lower_bound),
          upper_bound_(upper_bound),
          total_keys_in_interval_(upper_bound - lower_bound + 1) {
    }

    void ProgressManager::start_scan() {
        stats_.start_time = std::chrono::steady_clock::now();
        stats_.last_checkpoint_time = stats_.start_time;
        stats_.keys_processed_total = 0;
        stats_.current_position = lower_bound_;
        stats_.keys_found = 0;
        stats_.match_found = false;
    }

    void ProgressManager::update_progress(uint64_t keys_scanned_in_chunk, uint64_t current_chunk_end_key) {
        stats_.keys_processed_total += keys_scanned_in_chunk;
        stats_.current_position = current_chunk_end_key;
    }

    void ProgressManager::report_match(const std::string& priv_key_hex, const std::string& pub_key_compressed_hex, const std::string& address) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.match_found = true;
        stats_.keys_found++;
        stats_.found_private_key_hex = priv_key_hex;
        stats_.found_public_key_compressed_hex = pub_key_compressed_hex;
        stats_.found_address = address;
    }

    ScanStats ProgressManager::get_stats() const {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        return stats_;
    }

    double ProgressManager::calculate_keys_per_second() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - stats_.start_time).count();
        if (elapsed_seconds == 0) return 0.0;
        return static_cast<double>(stats_.keys_processed_total.load()) / elapsed_seconds;
    }

    std::string ProgressManager::format_time(long long seconds) const {
        long long hours = seconds / 3600;
        seconds %= 3600;
        long long minutes = seconds / 60;
        seconds %= 60;

        std::stringstream ss;
        ss << std::setfill('0') << std::setw(2) << hours << ":"
           << std::setfill('0') << std::setw(2) << minutes << ":"
           << std::setfill('0') << std::setw(2) << seconds;
        return ss.str();
    }

    void ProgressManager::display_dashboard() const {
        // Clear console for real-time update (platform-dependent, for Windows use system("cls"))
        // For cross-platform, a proper UI library would be needed. For now, just print.
        // std::cout << "\033[2J\033[1;1H"; // ANSI escape codes to clear screen and move cursor to top-left

        ScanStats current_stats = get_stats();

        auto now = std::chrono::steady_clock::now();
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - current_stats.start_time).count();

        double kps = calculate_keys_per_second();
        double mkps = kps / 1000000.0;

        uint64_t remaining_keys = total_keys_in_interval_ - (current_stats.current_position.load() - lower_bound_);
        long long estimated_remaining_seconds = 0;
        if (kps > 0) {
            estimated_remaining_seconds = static_cast<long long>(remaining_keys / kps);
        }

        double percent_covered = (static_cast<double>(current_stats.current_position.load() - lower_bound_) / total_keys_in_interval_) * 100.0;

        std::cout << "========================================================\n";
        std::cout << "           Bitcoin Puzzle #" << Config::PUZZLE_NUMBER << " CPU Scanner\n";
        std::cout << "========================================================\n";
        std::cout << "Puzzle Number:         " << Config::PUZZLE_NUMBER << "\n";
        std::cout << "Elapsed Time:          " << format_time(elapsed_seconds) << "\n";
        std::cout << "Keys Processed:        " << current_stats.keys_processed_total.load() << "\n";
        std::cout << "Keys Per Second:       " << std::fixed << std::setprecision(2) << kps << "\n";
        std::cout << "Millions of Keys/Sec:  " << std::fixed << std::setprecision(2) << mkps << "\n";
        std::cout << "Current Position:      0x" << std::hex << std::setw(16) << std::setfill('0') << current_stats.current_position.load() << std::dec << "\n";
        std::cout << "Percent of Interval:   " << std::fixed << std::setprecision(4) << percent_covered << "%\n";
        std::cout << "Est. Remaining Time:   " << format_time(estimated_remaining_seconds) << "\n";
        std::cout << "Active Threads:        " << Config::DEFAULT_WORKER_THREADS << " (compile-time constant)\n"; // TODO: Get actual active threads
        std::cout << "Checkpoint Status:     " << "Saving every " << Config::CHECKPOINT_INTERVAL_SECONDS << "s\n"; // TODO: Show actual status
        std::cout << "Memory Usage Estimate: " << "N/A (for this educational project)\n";
        std::cout << "========================================================\n";

        if (current_stats.match_found.load()) {
            std::cout << "\nMATCH FOUND!\n";
            std::cout << "Private Key (Hex):     " << current_stats.found_private_key_hex << "\n";
            std::cout << "Public Key (Comp.):    " << current_stats.found_public_key_compressed_hex << "\n";
            std::cout << "Derived Address:       " << current_stats.found_address << "\n";
            std::cout << "========================================================\n";
        }
    }

}
