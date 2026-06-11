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
        int puzzle_number
    )
        : lower_bound_(lower_bound),
          upper_bound_(upper_bound),
          target_hash160_(target_hash160),
          num_threads_(num_threads),
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
                    next_chunk_start_key_ = loaded_stats.current_position;
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
        ECC::Point current_point(worker_ecc_context);

        Types::UInt256 current_key_value;
        Types::PrivateKey priv_key;
        Types::PublicKeyCompressed pub_key_compressed;
        Types::Hash160 current_hash160;

        // Worker-specific checkpoint state (for resuming)
        Checkpoint::WorkerCheckpointState worker_checkpoint_state;
        worker_checkpoint_state.current_private_key_value = lower_bound_;

        // Chunking logic
        const uint64_t CHUNK_SIZE = 100000; // Process keys in chunks

        while (running_.load() && !progress_manager_.is_match_found()) {
            Types::UInt256 chunk_start_key;
            {
                std::lock_guard<std::mutex> lock(chunk_mutex_);
                chunk_start_key = next_chunk_start_key_;
                next_chunk_start_key_ += CHUNK_SIZE;
            }
            Types::UInt256 chunk_end_key = chunk_start_key + (CHUNK_SIZE - 1);
            if (chunk_end_key > upper_bound_) {
                chunk_end_key = upper_bound_;
            }

            if (chunk_start_key > upper_bound_) {
                break; // All keys processed
            }

            // Initialize current_point for this chunk
            priv_key = uint256_to_private_key(chunk_start_key);
            if (!current_point.init_from_private_key(priv_key)) {
                std::cerr << Config::current_time() << "Worker " << worker_id << ": Failed to initialize point from private key.\n";
                running_ = false; // Critical error, stop all
                break;
            }

            for (current_key_value = chunk_start_key; current_key_value <= chunk_end_key; ++current_key_value) {
                if (!running_.load() || progress_manager_.is_match_found()) {
                    break; // Stop if requested or match found by another thread
                }

                // Derive compressed public key
                if (!current_point.serialize_compressed(pub_key_compressed)) {
                    std::cerr << Config::current_time() << "Worker " << worker_id << ": Failed to serialize public key.\n";
                    running_ = false;
                    break;
                }

                // Compute HASH160
                Hashing::hash160(pub_key_compressed.data(), pub_key_compressed.size(), current_hash160.data());

                // Compare against target
                if (std::equal(current_hash160.begin(), current_hash160.end(), target_hash160_.begin())) {
                    std::cout << Config::current_time() << "Match found by worker " << worker_id << "!\n";
                    progress_manager_.report_match(
                        private_key_to_hex(uint256_to_private_key(current_key_value)),
                        public_key_to_hex(pub_key_compressed),
                        hash160_to_address(current_hash160)
                    );
                    running_ = false; // Signal other threads to stop
                    checkpoint_cv_.notify_all(); // Wake up checkpoint thread
                    break;
                }

                // Incremental point walking: P = P + G
                if (current_key_value < upper_bound_) { // Avoid adding G if it's the last key
                    if (!current_point.add_generator()) {
                        std::cerr << Config::current_time() << "Worker " << worker_id << ": Failed to add generator point.\n";
                        running_ = false;
                        break;
                    }
                }

                // Update worker's current private key for checkpointing
                worker_checkpoint_state.current_private_key_value = current_key_value;
            }

            progress_manager_.update_progress((chunk_end_key - chunk_start_key) + 1, chunk_end_key);
        }
    }

    void ScannerEngine::checkpoint_thread_func() {
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
            int hours = elapsed_seconds / 3600;
            int minutes = (elapsed_seconds % 3600) / 60;
            int seconds = elapsed_seconds % 60;
            std::string time_str = std::to_string(hours) + "h " + std::to_string(minutes) + "m " + std::to_string(seconds) + "s";

            std::cout << Config::current_time() << "Checkpoint saved at key: " << current_stats.current_position.to_hex() 
                      << " | Elapsed: " << time_str << "\n";
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
