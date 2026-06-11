#include "progress_manager.h"
#include "config.h"
#include <iostream>
#include <iomanip>
#include <cmath>

namespace Progress {

    ProgressManager::ProgressManager(Types::UInt256 lower_bound, Types::UInt256 upper_bound, int puzzle_number, int num_threads)
        : lower_bound_(lower_bound),
          upper_bound_(upper_bound),
          total_keys_in_interval_(upper_bound - lower_bound + 1),
          puzzle_number_(puzzle_number),
          num_threads_(num_threads) {
    }

    void ProgressManager::start_scan() {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.start_time = std::chrono::steady_clock::now();
        stats_.last_checkpoint_time = stats_.start_time;
        stats_.keys_processed_total = 0;
        stats_.current_position = lower_bound_;
        stats_.keys_found = 0;
        stats_.match_found = false;
    }

    void ProgressManager::update_progress(uint64_t keys_scanned_in_chunk, Types::UInt256 current_chunk_end_key) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.keys_processed_total += keys_scanned_in_chunk;
        stats_.current_position = current_chunk_end_key;
    }

    void ProgressManager::add_keys_processed(uint64_t keys_processed) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.keys_processed_total += keys_processed;
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
        ScanStats current_stats = get_stats();
        return static_cast<double>(current_stats.keys_processed_total.q0) / elapsed_seconds;
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
        // std::cout << Config::current_time() << "\033[2J\033[1;1H"; // ANSI escape codes to clear screen and move cursor to top-left

        ScanStats current_stats = get_stats();

        auto now = std::chrono::steady_clock::now();
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - current_stats.start_time).count();

        double kps = calculate_keys_per_second();
        double mkps = kps / 1000000.0;

        uint64_t remaining_keys = (total_keys_in_interval_ - (current_stats.current_position - lower_bound_));
        long long estimated_remaining_seconds = 0;
        if (kps > 0) {
            estimated_remaining_seconds = static_cast<long long>(remaining_keys / kps);
        }

        double percent_covered = (static_cast<double>((current_stats.current_position - lower_bound_)) / static_cast<double>(total_keys_in_interval_.q0)) * 100.0; // Approximation

        std::cout << Config::current_time() << "========================================================\n";
        std::cout << Config::current_time() << "           Bitcoin Puzzle #" << puzzle_number_ << " CPU Scanner\n";
        std::cout << Config::current_time() << "========================================================\n";
        std::cout << Config::current_time() << "Puzzle Number:         " << puzzle_number_ << "\n";
        std::cout << Config::current_time() << "Elapsed Time:          " << format_time(elapsed_seconds) << "\n";
        std::cout << Config::current_time() << "Keys Processed:        " << current_stats.keys_processed_total.q0 << "\n";
        std::cout << Config::current_time() << "Keys Per Second:       " << std::fixed << std::setprecision(2) << kps << "\n";
        std::cout << Config::current_time() << "Millions of Keys/Sec:  " << std::fixed << std::setprecision(2) << mkps << "\n";
        std::cout << Config::current_time() << "Current Position:      0x" << current_stats.current_position.to_hex() << "\n";
        std::cout << Config::current_time() << "Percent of Interval:   " << std::fixed << std::setprecision(4) << percent_covered << "%\n";
        std::cout << Config::current_time() << "Est. Remaining Time:   " << format_time(estimated_remaining_seconds) << "\n";
        std::cout << Config::current_time() << "Active Threads:        " << num_threads_ << "\n";
        std::cout << Config::current_time() << "Checkpoint Status:     " << "Saving every " << Config::CHECKPOINT_INTERVAL_SECONDS << "s\n";
        std::cout << Config::current_time() << "Memory Usage Estimate: " << "N/A (for this educational project)\n";
        std::cout << Config::current_time() << "========================================================\n";

        if (current_stats.match_found) {
            std::cout << Config::current_time() << "\nMATCH FOUND!\n";
            std::cout << Config::current_time() << "Private Key (Hex):     " << current_stats.found_private_key_hex << "\n";
            std::cout << Config::current_time() << "Public Key (Comp.):    " << current_stats.found_public_key_compressed_hex << "\n";
            std::cout << Config::current_time() << "Derived Address:       " << current_stats.found_address << "\n";
            std::cout << Config::current_time() << "========================================================\n";
        }
    }

}
