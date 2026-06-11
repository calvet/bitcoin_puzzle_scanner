#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>

#include "config.h"
#include "puzzles.h"
#include "types.h"
#include "scanner_engine.h"
#include "checkpoint_manager.h"
#include <iomanip>

void set_bit(Types::UInt256& num, int bit) {
    if (bit < 64) num.q0 |= (1ULL << bit);
    else if (bit < 128) num.q1 |= (1ULL << (bit - 64));
    else if (bit < 192) num.q2 |= (1ULL << (bit - 128));
    else if (bit < 256) num.q3 |= (1ULL << (bit - 192));
}

Types::UInt256 get_lower_bound(int N) {
    Types::UInt256 res;
    if (N > 0) {
        set_bit(res, N - 1);
    }
    return res;
}

Types::UInt256 get_upper_bound(int N) {
    Types::UInt256 res;
    for (int i = 0; i < N; ++i) {
        set_bit(res, i);
    }
    return res;
}

int main() {
    std::cout << Config::current_time() << "Welcome to Bitcoin Puzzle Scanner!\n";

    // Check for existing checkpoints
    auto existing_checkpoints = Checkpoint::CheckpointManager::get_all_checkpoints(Config::CHECKPOINT_DIR);
    if (!existing_checkpoints.empty()) {
        std::cout << Config::current_time() << "--- Existing Checkpoints ---\n";
        for (const auto& cp : existing_checkpoints) {
            double range_total = cp.upper_bound.get_double() - cp.lower_bound.get_double();
            double current_prog = cp.current_position.get_double() - cp.lower_bound.get_double();
            double percentage = 0.0;
            if (range_total > 0) {
                percentage = (current_prog / range_total) * 100.0;
            }
            // Add bounds check in case current_prog is negative or > range_total
            if (percentage < 0.0) percentage = 0.0;
            if (percentage > 100.0) percentage = 100.0;

            std::cout << Config::current_time() << "  Puzzle #" << cp.puzzle_number 
                      << " | Progress: " << std::fixed << std::setprecision(4) << percentage << "%\n";
        }
        std::cout << Config::current_time() << "----------------------------\n";
    }

    std::cout << Config::current_time() << "Choose the Puzzle number (1 to 160)\n";
    std::cout << Config::current_time() << "[Note: Puzzle #71 is currently the easiest available with balance!]\n";
    std::cout << Config::current_time() << "Puzzle [Default 71]: ";
    std::string input;
    std::getline(std::cin, input);
    
    int puzzle_num = 71;
    if (!input.empty()) {
        try { puzzle_num = std::stoi(input); } catch (...) {}
    }
    if (puzzle_num < 1) puzzle_num = 1;
    if (puzzle_num > 160) puzzle_num = 160;

    const auto& puzzles = Config::GetPuzzles();
    if (puzzles.find(puzzle_num) == puzzles.end()) {
        std::cerr << Config::current_time() << "Error: Information for Puzzle " << puzzle_num << " not found.\n";
        return 1;
    }
    
    auto puzzle_info = puzzles.at(puzzle_num);
    std::string target_hash_hex = puzzle_info.hash160;
    std::string target_address = puzzle_info.address;

    if (puzzle_info.solved) {
        std::cout << Config::current_time() << "[WARNING] Puzzle #" << puzzle_num << " is already marked as SOLVED in the Puzzle Website!\n";
    }

    int max_threads = std::thread::hardware_concurrency();
    if (max_threads == 0) max_threads = 4;
    int num_threads = max_threads;
    std::cout << Config::current_time() << "How many threads do you want to use? (Max: " << max_threads << ") [Default: " << max_threads << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        try { num_threads = std::stoi(input); } catch (...) {}
    }
    if (num_threads < 1) num_threads = 1;
    if (num_threads > max_threads) num_threads = max_threads;

    std::cout << Config::current_time() << "Checkpoint interval in seconds [Default 15]: ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        try { Config::CHECKPOINT_INTERVAL_SECONDS = std::stoi(input); } catch (...) {}
    }
    if (Config::CHECKPOINT_INTERVAL_SECONDS < 1) Config::CHECKPOINT_INTERVAL_SECONDS = 15;
    if (Config::CHECKPOINT_INTERVAL_SECONDS > 300) Config::CHECKPOINT_INTERVAL_SECONDS = 300;

    int mode_selection = 1;
    std::cout << Config::current_time() << "Choose Scan Mode:\n";
    std::cout << Config::current_time() << "[1] Sequential (Default)\n";
    std::cout << Config::current_time() << "[2] Random\n";
    std::cout << Config::current_time() << "Mode [Default 1]: ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        try { mode_selection = std::stoi(input); } catch (...) {}
    }
    Scanner::ScanMode scan_mode = (mode_selection == 2) ? Scanner::ScanMode::RANDOM : Scanner::ScanMode::SEQUENTIAL;

    int max_pause = 0;
    std::cout << Config::current_time() << "Max random pause between blocks (0 to 60s) [Default 0]: ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        try { max_pause = std::stoi(input); } catch (...) {}
    }
    if (max_pause < 0) max_pause = 0;
    if (max_pause > 60) max_pause = 60;

    std::cout << Config::current_time() << "Enable verbose mode? (prints active block per thread) [y/N] [Default N]: ";
    std::getline(std::cin, input);
    if (!input.empty() && (input == "y" || input == "Y")) {
        Config::VERBOSE_MODE = true;
    }

    if (scan_mode == Scanner::ScanMode::RANDOM) {
        std::cout << Config::current_time() << "[WARNING] Random mode enabled. Checkpoints will NOT save your resume position, only total keys processed and elapsed time!\n";
    }

    std::string telegram_token = "";
    std::string telegram_chat_id = "";
    std::cout << Config::current_time() << "Enable Telegram notifications? [y/N] [Default N]: ";
    std::getline(std::cin, input);
    if (!input.empty() && (input == "y" || input == "Y")) {
        std::cout << Config::current_time() << "Enter Telegram Bot Token: ";
        std::getline(std::cin, telegram_token);
        std::cout << Config::current_time() << "Enter Telegram Chat ID (e.g., from @userinfobot): ";
        std::getline(std::cin, telegram_chat_id);

        if (!telegram_token.empty() && !telegram_chat_id.empty()) {
            std::cout << Config::current_time() << "Sending test message to Telegram...\n";
#ifdef _WIN32
            std::string null_dev = " > NUL 2>&1";
#else
            std::string null_dev = " > /dev/null 2>&1";
#endif
            std::string cmd = "curl -s -X POST https://api.telegram.org/bot" + telegram_token + "/sendMessage -d chat_id=" + telegram_chat_id + " -d text=\"Bitcoin Puzzle Scanner: Notifications enabled successfully!\"" + null_dev;
            std::system(cmd.c_str());
        } else {
            std::cout << Config::current_time() << "Telegram token or chat ID is empty. Notifications disabled.\n";
            telegram_token = "";
            telegram_chat_id = "";
        }
    }

    Types::UInt256 lower_bound = get_lower_bound(puzzle_num);
    Types::UInt256 upper_bound = get_upper_bound(puzzle_num);

    std::cout << Config::current_time() << "Starting Scanner...\n";
    std::cout << Config::current_time() << "Puzzle: #" << puzzle_num << " (Status: " << (puzzle_info.solved ? "SOLVED" : "NOT SOLVED") << ")\n";
    std::cout << Config::current_time() << "Target Address: " << target_address << "\n";
    std::cout << Config::current_time() << "Target HASH160: " << target_hash_hex << "\n";
    std::cout << Config::current_time() << "Threads: " << num_threads << "\n";
    std::cout << Config::current_time() << "Scan Mode: " << (scan_mode == Scanner::ScanMode::SEQUENTIAL ? "Sequential" : "Random") << "\n";
    if (max_pause > 0) {
        std::cout << Config::current_time() << "Max Random Pause: " << max_pause << "s\n";
    }
    std::cout << Config::current_time() << "Verbose Mode:     " << (Config::VERBOSE_MODE ? "Enabled" : "Disabled") << "\n";
    std::cout << Config::current_time() << "Telegram Notifs:  " << (!telegram_token.empty() ? "Enabled" : "Disabled") << "\n";
    std::cout << Config::current_time() << "Search Range: 0x" << lower_bound.to_hex() << " to 0x" << upper_bound.to_hex() << "\n";

    Types::Hash160 target_hash160_bytes;
    // Convert hex string to bytes for target_hash160_bytes
    for (size_t i = 0; i < target_hash_hex.length() && i < 40; i += 2) {
        std::string byteString = target_hash_hex.substr(i, 2);
        target_hash160_bytes[i / 2] = static_cast<uint8_t>(std::stoul(byteString, nullptr, 16));
    }

    try {
        Scanner::ScannerEngine engine(
            lower_bound,
            upper_bound,
            target_hash160_bytes,
            num_threads,
            puzzle_num,
            scan_mode,
            max_pause
        );
        engine.start();

        auto stats = engine.get_scan_stats();
        if (stats.match_found) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - stats.start_time).count();
            int hours = static_cast<int>(elapsed_seconds / 3600);
            int minutes = static_cast<int>((elapsed_seconds % 3600) / 60);
            int seconds = static_cast<int>(elapsed_seconds % 60);
            std::string time_str = std::to_string(hours) + "h " + std::to_string(minutes) + "m " + std::to_string(seconds) + "s";

            Types::UInt256 found_key_val = Types::UInt256::from_hex(stats.found_private_key_hex);
            double range_total = upper_bound.get_double() - lower_bound.get_double();
            double key_pos = found_key_val.get_double() - lower_bound.get_double();
            double pos_percentage = 0.0;
            if (range_total > 0) {
                pos_percentage = (key_pos / range_total) * 100.0;
            }
            if (pos_percentage < 0.0) pos_percentage = 0.0;
            if (pos_percentage > 100.0) pos_percentage = 100.0;

            std::cout << Config::current_time() << "========================================================\n";
            std::cout << Config::current_time() << "MATCH FOUND!\n";
            std::cout << Config::current_time() << "Private Key (Hex):     " << stats.found_private_key_hex << "\n";
            std::cout << Config::current_time() << "Public Key (Comp.):    " << stats.found_public_key_compressed_hex << "\n";
            std::cout << Config::current_time() << "Derived Address:       " << stats.found_address << "\n";
            std::cout << Config::current_time() << "Time Taken:            " << time_str << "\n";
            std::cout << Config::current_time() << "Position in Range:     " << std::fixed << std::setprecision(4) << pos_percentage << "%\n";
            std::cout << Config::current_time() << "========================================================\n";

            std::string filename = "FOUND_PUZZLE_" + std::to_string(puzzle_num) + ".txt";
            std::ofstream out_file(filename);
            if (out_file.is_open()) {
                out_file << Config::current_time() << "MATCH FOUND FOR PUZZLE #" << puzzle_num << "\n";
                out_file << Config::current_time() << "Private Key (Hex):     " << stats.found_private_key_hex << "\n";
                out_file << Config::current_time() << "Public Key (Comp.):    " << stats.found_public_key_compressed_hex << "\n";
                out_file << Config::current_time() << "Derived Address:       " << stats.found_address << "\n";
                out_file << Config::current_time() << "Time Taken:            " << time_str << "\n";
                out_file << Config::current_time() << "Position in Range:     " << std::fixed << std::setprecision(4) << pos_percentage << "%\n";
                out_file.close();
                std::cout << Config::current_time() << "Details saved to " << filename << "\n";

                std::string checkpoint_file = "checkpoints/puzzle_" + std::to_string(puzzle_num) + ".checkpoint";
                std::string used_file = "checkpoints/puzzle_" + std::to_string(puzzle_num) + ".checkpoint.used";
                if (std::rename(checkpoint_file.c_str(), used_file.c_str()) == 0) {
                    std::cout << Config::current_time() << "Checkpoint renamed to " << used_file << "\n";
                }
            } else {
                std::cerr << Config::current_time() << "Failed to save match details to file.\n";
            }

            if (!telegram_token.empty() && !telegram_chat_id.empty()) {
                std::string msg = "BINGO! The scanner found the result for Puzzle %23" + std::to_string(puzzle_num) + "! Check the file " + filename + " on the server immediately.";
#ifdef _WIN32
                std::string null_dev = " > NUL 2>&1";
#else
                std::string null_dev = " > /dev/null 2>&1";
#endif
                std::string cmd = "curl -s -X POST https://api.telegram.org/bot" + telegram_token + "/sendMessage -d chat_id=" + telegram_chat_id + " -d text=\"" + msg + "\"" + null_dev;
                std::system(cmd.c_str());
                std::cout << Config::current_time() << "Telegram notification sent!\n";
            }
        }

    } catch (const std::exception& e) {
        std::cerr << Config::current_time() << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << Config::current_time() << "Scanner finished.\n";
    std::cout << Config::current_time() << "Press Enter to exit...";
    std::string dummy;
    std::getline(std::cin, dummy);
    
    return 0;
}
