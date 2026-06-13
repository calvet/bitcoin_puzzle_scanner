#include "scanner_engine.h"
#include "config.h"
#include "sha256.h"
#include "ripemd160.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <chrono>

// Forward declaration for Base58 encoding (will be implemented or included separately)
std::string to_base58(const std::vector<uint8_t>& data);

namespace Scanner {

    ScannerEngine::ScannerEngine(
        Types::UInt256 lower_bound,
        Types::UInt256 upper_bound,
        const Types::Hash160& target_hash160,
        int num_threads,
        int puzzle_number,
        ScanMode mode,
        int max_pause_seconds
    )
        : lower_bound_(lower_bound),
          upper_bound_(upper_bound),
          target_hash160_(target_hash160),
          num_threads_(num_threads),
          mode_(mode),
          max_pause_seconds_(max_pause_seconds),
          running_(false),
          next_chunk_start_key_(lower_bound),
          progress_manager_(lower_bound, upper_bound, puzzle_number, num_threads),
          checkpoint_manager_(Config::CHECKPOINT_DIR, lower_bound, upper_bound, puzzle_number) {

        // Try to load checkpoint
        if (checkpoint_manager_.checkpoint_exists()) {
            Progress::ScanStats loaded_stats;
            std::vector<Checkpoint::WorkerCheckpointState> loaded_worker_states;
            if (checkpoint_manager_.load_latest_checkpoint(loaded_stats, loaded_worker_states)) {
                if (loaded_stats.current_position >= lower_bound_ && loaded_stats.current_position <= upper_bound_) {
                    std::cout << Config::current_time() << "Resuming from checkpoint. Keys processed: " << loaded_stats.keys_processed_total.q0 << "\n";
                    progress_manager_.start_scan(); // Re-initialize start time
                    progress_manager_.update_progress(loaded_stats.keys_processed_total.q0, loaded_stats.current_position);
                    if (mode_ == ScanMode::SEQUENTIAL) {
                        next_chunk_start_key_ = loaded_stats.current_position;
                    } else {
                        std::cout << Config::current_time() << "Random mode selected: ignoring saved position, but keeping total processed count.\n";
                    }
                    // TODO: Distribute loaded_worker_states to individual workers if needed
                } else {
                    std::cerr << Config::current_time() << "Checkpoint is for a different puzzle or range. Starting new scan.\n";
                    progress_manager_.start_scan();
                }
            } else {
                std::cerr << Config::current_time() << "Failed to load checkpoint. Starting new scan.\n";
                progress_manager_.start_scan();
            }
        } else {
            progress_manager_.start_scan();
        }
    }

    ScannerEngine::~ScannerEngine() {
        stop();
    }

    void ScannerEngine::start() {
        running_ = true;
        std::cout << Config::current_time() << "Starting scanner with " << num_threads_ << " threads.\n";

        // Start worker threads
        for (int i = 0; i < num_threads_; ++i) {
            workers_.emplace_back(&ScannerEngine::worker_thread_func, this, i);
        }

        // Start checkpoint thread
        workers_.emplace_back(&ScannerEngine::checkpoint_thread_func, this);

        // Wait for all threads to finish (or be stopped)
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void ScannerEngine::stop() {
        if (running_) {
            running_ = false;
            // Notify checkpoint thread to stop if it's waiting
            checkpoint_cv_.notify_all();
            std::cout << Config::current_time() << "Stopping scanner...\n";
        }
    }

    Types::PrivateKey ScannerEngine::uint256_to_private_key(const Types::UInt256& key_value) {
        Types::PrivateKey priv_key;
        priv_key.fill(0);
        for (int i = 0; i < 8; ++i) {
            priv_key[31 - i] = (key_value.q0 >> (i * 8)) & 0xFF;
            priv_key[23 - i] = (key_value.q1 >> (i * 8)) & 0xFF;
            priv_key[15 - i] = (key_value.q2 >> (i * 8)) & 0xFF;
            priv_key[7 - i] = (key_value.q3 >> (i * 8)) & 0xFF;
        }
        return priv_key;
    }

    std::string ScannerEngine::private_key_to_hex(const Types::PrivateKey& priv_key) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (uint8_t byte : priv_key) {
            ss << std::setw(2) << static_cast<int>(byte);
        }
        return ss.str();
    }

    std::string ScannerEngine::public_key_to_hex(const Types::PublicKeyCompressed& pub_key) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (uint8_t byte : pub_key) {
            ss << std::setw(2) << static_cast<int>(byte);
        }
        return ss.str();
    }

    std::string ScannerEngine::hash160_to_hex(const Types::Hash160& hash) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (uint8_t byte : hash) {
            ss << std::setw(2) << static_cast<int>(byte);
        }
        return ss.str();
    }

    std::string ScannerEngine::hash160_to_address(const Types::Hash160& hash) {
        // Bitcoin address format: 1-byte version + 20-byte HASH160 + 4-byte checksum
        std::vector<uint8_t> address_data;
        address_data.push_back(0x00); // Mainnet version byte
        address_data.insert(address_data.end(), hash.begin(), hash.end());

        // Calculate checksum (double SHA256 of the above)
        Types::Sha256 sha256_1;
        Hashing::sha256_bytes(address_data.data(), address_data.size(), sha256_1.data());
        Types::Sha256 sha256_2;
        Hashing::sha256_bytes(sha256_1.data(), sha256_1.size(), sha256_2.data());

        address_data.insert(address_data.end(), sha256_2.begin(), sha256_2.begin() + 4); // First 4 bytes of double SHA256

        return to_base58(address_data);
    }

    void ScannerEngine::worker_thread_func(int worker_id) {
        // Each worker needs its own ECC::Context to avoid thread-safety issues with secp256k1
        ECC::Context worker_ecc_context;
        ECC::Point p[4] = {
            ECC::Point(worker_ecc_context),
            ECC::Point(worker_ecc_context),
            ECC::Point(worker_ecc_context),
            ECC::Point(worker_ecc_context)
        };

        Types::UInt256 current_key_value;
        Types::PrivateKey priv_key;
        Types::PublicKeyCompressed pub_key_compressed;

        // Worker-specific checkpoint state (for resuming)
        Checkpoint::WorkerCheckpointState worker_checkpoint_state;
        worker_checkpoint_state.current_private_key_value = lower_bound_;

        // Chunking logic
        const uint64_t CHUNK_SIZE = 100000; // Process keys in chunks

        while (running_.load() && !progress_manager_.is_match_found()) {
            Types::UInt256 chunk_start_key;
            {
                std::lock_guard<std::mutex> lock(chunk_mutex_);
                if (mode_ == ScanMode::RANDOM) {
                    Types::UInt256 upper_random = upper_bound_;
                    if (upper_random > lower_bound_ + CHUNK_SIZE) {
                        upper_random = upper_bound_.subtract(CHUNK_SIZE);
                    }
                    chunk_start_key = Types::UInt256::generate_random(lower_bound_, upper_random);
                } else {
                    chunk_start_key = next_chunk_start_key_;
                    next_chunk_start_key_ += CHUNK_SIZE;
                }
            }
            Types::UInt256 chunk_end_key = chunk_start_key + (CHUNK_SIZE - 1);
            if (chunk_end_key > upper_bound_) {
                chunk_end_key = upper_bound_;
            }

            if (chunk_start_key > upper_bound_) {
                break; // All keys processed
            }

            if (Config::VERBOSE_MODE) {
                std::cout << Config::current_time()
                          << "Worker " << worker_id
                          << ": processing block 0x" << chunk_start_key.to_hex()
                          << " to 0x" << chunk_end_key.to_hex() << "\n";
            }

            // Initialize 4 points for this chunk
            for (int i = 0; i < 4; ++i) {
                priv_key = uint256_to_private_key(chunk_start_key + i);
                if (!p[i].init_from_private_key(priv_key)) {
                    std::cerr << Config::current_time() << "Worker " << worker_id << ": Failed to initialize point from private key.\n";
                    running_ = false;
                    break;
                }
            }
            if (!running_.load()) break;

            for (current_key_value = chunk_start_key; current_key_value <= chunk_end_key; current_key_value += 4) {
                if (!running_.load() || progress_manager_.is_match_found()) {
                    break; // Stop if requested or match found by another thread
                }

                uint8_t h0[20], h1[20], h2[20], h3[20];
                worker_ecc_context.get()->GetHash160(P2PKH, true, p[0].get_raw(), p[1].get_raw(), p[2].get_raw(), p[3].get_raw(), h0, h1, h2, h3);

                uint8_t* hashes[4] = { h0, h1, h2, h3 };
                bool match = false;
                int match_index = -1;

                for (int i = 0; i < 4; ++i) {
                    if (current_key_value + i <= chunk_end_key) {
                        if (std::equal(hashes[i], hashes[i] + 20, target_hash160_.begin())) {
                            match = true;
                            match_index = i;
                            break;
                        }
                    }
                }

                if (match) {
                    std::cout << Config::current_time() << "Worker " << worker_id << ": Match found!\n";
                    Types::UInt256 match_key = current_key_value + match_index;
                    p[match_index].serialize_compressed(pub_key_compressed);
                    
                    Types::Hash160 hash_matched;
                    std::copy(hashes[match_index], hashes[match_index] + 20, hash_matched.begin());

                    progress_manager_.report_match(
                        private_key_to_hex(uint256_to_private_key(match_key)),
                        public_key_to_hex(pub_key_compressed),
                        hash160_to_address(hash_matched)
                    );
                    running_ = false; // Signal other threads to stop
                    checkpoint_cv_.notify_all(); // Wake up checkpoint thread
                    break;
                }

                // Incremental point walking: P = P + 4G
                if (current_key_value + 3 < upper_bound_) { 
                    if (!ECC::batch_add_4G(p[0], p[1], p[2], p[3], worker_ecc_context)) {
                        std::cerr << Config::current_time() << "Worker " << worker_id << ": Failed to batch add 4G.\n";
                        running_ = false;
                        break;
                    }
                }

                // Update worker's current private key for checkpointing
                worker_checkpoint_state.current_private_key_value = current_key_value;
            }

            if (mode_ == ScanMode::SEQUENTIAL) {
                progress_manager_.update_progress((chunk_end_key - chunk_start_key) + 1, chunk_end_key);
            } else {
                progress_manager_.add_keys_processed((chunk_end_key - chunk_start_key) + 1);
            }

            // Random pause logic to alleviate CPU load (1 in 10 chance to trigger)
            if (max_pause_seconds_ > 0 && running_.load()) {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> chance_dist(1, 20);
                
                if (chance_dist(gen) == 1) { // 5% chance to pause
                    std::uniform_int_distribution<> dist(1, max_pause_seconds_);
                    int pause_time = dist(gen);
                    if (pause_time > 0) {
                        if (Config::VERBOSE_MODE) {
                            std::cout << Config::current_time() << "Worker " << worker_id << ": Pausing for " << pause_time << " seconds.\n";
                        }
                        // Sleep in 100ms increments to stay responsive to stop requests
                        for (int s = 0; s < pause_time * 10 && running_.load(); ++s) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }
                    }
                }
            }
        }
    }

    static std::string format_large_number(double num) {
        const char* suffixes[] = {"", "K", "M", "B", "T", "Qa", "Qi", "Sx", "Sp", "Oc", "No", "Dc"};
        int suffix_index = 0;
        while (num >= 1000.0 && suffix_index < 11) {
            num /= 1000.0;
            suffix_index++;
        }
        std::stringstream ss;
        if (suffix_index == 0) {
            ss << static_cast<uint64_t>(num);
        } else {
            ss << std::fixed << std::setprecision(1) << num << suffixes[suffix_index];
        }
        std::string res = ss.str();
        std::string suffix = suffixes[suffix_index];
        size_t suffix_len = suffix.length();
        if (res.length() >= suffix_len + 2 && res.substr(res.length() - suffix_len - 2, 2) == ".0") {
            res.erase(res.length() - suffix_len - 2, 2);
        }
        return res;
    }

    void ScannerEngine::checkpoint_thread_func() {
        double last_keys_processed = progress_manager_.get_stats().keys_processed_total.get_double();
        auto last_time = std::chrono::steady_clock::now();

        while (running_.load() && !progress_manager_.is_match_found()) {
            std::unique_lock<std::mutex> lock(checkpoint_mutex_);
            checkpoint_cv_.wait_for(lock, std::chrono::seconds(Config::CHECKPOINT_INTERVAL_SECONDS), [&] {
                return !running_.load() || progress_manager_.is_match_found();
            });

            if (!running_.load() || progress_manager_.is_match_found()) {
                break;
            }

            // Save checkpoint
            Progress::ScanStats current_stats = progress_manager_.get_stats();
            std::vector<Checkpoint::WorkerCheckpointState> worker_states; // TODO: Populate with actual worker states
            checkpoint_manager_.save_checkpoint(current_stats, worker_states);

            auto now = std::chrono::steady_clock::now();
            auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - current_stats.start_time).count();
            int hours = static_cast<int>(elapsed_seconds / 3600);
            int minutes = static_cast<int>((elapsed_seconds % 3600) / 60);
            int seconds = static_cast<int>(elapsed_seconds % 60);
            std::string time_str = std::to_string(hours) + "h " + std::to_string(minutes) + "m " + std::to_string(seconds) + "s";

            double range_total = upper_bound_.get_double() - lower_bound_.get_double();
            double keys_processed = current_stats.keys_processed_total.get_double();
            double percentage = 0.0;
            if (range_total > 0) {
                percentage = (keys_processed / range_total) * 100.0;
            }
            if (percentage > 100.0) percentage = 100.0;

            double delta_time = std::chrono::duration<double>(now - last_time).count();
            double delta_keys = keys_processed - last_keys_processed;
            double keys_per_sec = delta_time > 0 ? (delta_keys / delta_time) : 0.0;

            last_keys_processed = keys_processed;
            last_time = now;

            std::string keys_str = format_large_number(keys_processed) + " / " + format_large_number(range_total);
            std::string kps_str = format_large_number(keys_per_sec) + "/s";

            if (mode_ != ScanMode::RANDOM) {
                std::cout << Config::current_time() << "Checkpoint saved at key: " << current_stats.current_position.to_hex() 
                          << " | Elapsed: " << time_str 
                          << " | Keys: " << keys_str
                          << " | Speed: " << kps_str
                          << " | Progress: " << std::fixed << std::setprecision(4) << percentage << "%\n";
            } else {
                std::cout << Config::current_time() << "Checkpoint saved"
                          << " | Elapsed: " << time_str 
                          << " | Keys: " << keys_str
                          << " | Speed: " << kps_str
                          << " | Progress: " << std::fixed << std::setprecision(4) << percentage << "%\n";
            }
        }
    }

}

// Basic Base58 encoding implementation (placeholder, consider a more robust library for production)
// This implementation is simplified and might not handle all edge cases or be fully optimized.
// For a production-grade solution, integrate a well-tested Base58 library.

static const char* BASE58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

std::string to_base58(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return "";
    }

    // Count leading zeros
    int zeros = 0;
    while (zeros < data.size() && data[zeros] == 0) {
        ++zeros;
    }

    // Convert binary to base58
    std::vector<uint8_t> b58_digits;
    std::vector<uint8_t> temp_data = data;

    while (!temp_data.empty()) {
        int carry = 0;
        std::vector<uint8_t> next_temp_data;
        for (uint8_t byte : temp_data) {
            int digit = byte + carry * 256;
            next_temp_data.push_back(static_cast<uint8_t>(digit / 58));
            carry = digit % 58;
        }
        b58_digits.push_back(static_cast<uint8_t>(carry));

        // Remove leading zeros from next_temp_data
        auto it = next_temp_data.begin();
        while (it != next_temp_data.end() && *it == 0) {
            ++it;
        }
        temp_data.assign(it, next_temp_data.end());
    }

    // Add leading '1's for original leading zeros
    std::string result;
    for (int i = 0; i < zeros; ++i) {
        result += BASE58_ALPHABET[0];
    }

    // Reverse and append digits
    std::reverse(b58_digits.begin(), b58_digits.end());
    for (uint8_t digit : b58_digits) {
        result += BASE58_ALPHABET[digit];
    }

    return result;
}
