#include "gtest/gtest.h"
#include "config.h"
#include "types.h"
#include "sha256.h"
#include "ripemd160.h"
#include "ecc.h"
#include "scanner_engine.h"
#include "progress_manager.h"
#include "checkpoint_manager.h"

#include <array>
#include <string>
#include <vector>
#include <numeric>

// Helper to convert hex string to byte array
Types::Hash160 hex_to_hash160(const std::string& hex_str) {
    Types::Hash160 hash;
    for (size_t i = 0; i < hex_str.length(); i += 2) {
        std::string byteString = hex_str.substr(i, 2);
        hash[i / 2] = static_cast<uint8_t>(std::stoul(byteString, nullptr, 16));
    }
    return hash;
}

// Helper to convert hex string to compressed public key
Types::PublicKeyCompressed hex_to_pubkey(const std::string& hex_str) {
    Types::PublicKeyCompressed pubkey;
    for (size_t i = 0; i < hex_str.length(); i += 2) {
        std::string byteString = hex_str.substr(i, 2);
        pubkey[i / 2] = static_cast<uint8_t>(std::stoul(byteString, nullptr, 16));
    }
    return pubkey;
}

// Test HASH160 generation
TEST(HashingTest, HASH160Generation) {
    // Test vector: HASH160 of empty string
    // SHA256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    // RIPEMD160(SHA256("")) = b472a26670bd3784c65326673f1140081bb22014
    std::string input_data = "";
    Types::Hash160 expected_hash160 = hex_to_hash160("b472a26670bd3784c65326673f1140081bb22014");
    Types::Hash160 actual_hash160;

    Hashing::hash160(input_data.data(), input_data.length(), actual_hash160.data());

    ASSERT_EQ(expected_hash160, actual_hash160);

    // Test vector: HASH160 of "hello world"
    // SHA256("hello world") = a591a6d40bf420404a011733cfb7b190d62c65bf0bcda32b57b277fe9ad9f90e
    // RIPEMD160(SHA256("hello world")) = 98c615784ccb5fe59c8c5793ff04927a082ba403
    input_data = "hello world";
    expected_hash160 = hex_to_hash160("98c615784ccb5fe59c8c5793ff04927a082ba403");
    Hashing::hash160(input_data.data(), input_data.length(), actual_hash160.data());
    ASSERT_EQ(expected_hash160, actual_hash160);
}

// Test Compressed Public Key Generation and Incremental Point Walking
TEST(ECCTest, CompressedPublicKeyAndIncrementalWalking) {
    ECC::Context ecc_ctx;
    ECC::Point point(ecc_ctx);

    // Test with private key 1 (0x01)
    Types::PrivateKey priv_key_one;
    priv_key_one.fill(0);
    priv_key_one[31] = 1;

    ASSERT_TRUE(point.init_from_private_key(priv_key_one));

    Types::PublicKeyCompressed pub_key_one;
    ASSERT_TRUE(point.serialize_compressed(pub_key_one));

    // Expected compressed public key for private key 1 (from known sources)
    // 0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798
    Types::PublicKeyCompressed expected_pub_key_one = hex_to_pubkey("0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798");
    ASSERT_EQ(expected_pub_key_one, pub_key_one);

    // Test incremental point walking (P = P + G)
    // Start with private key 1, add G to get public key for private key 2
    ASSERT_TRUE(point.add_generator());
    Types::PublicKeyCompressed pub_key_two;
    ASSERT_TRUE(point.serialize_compressed(pub_key_two));

    // Expected compressed public key for private key 2 (from known sources)
    // 03c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
    Types::PublicKeyCompressed expected_pub_key_two = hex_to_pubkey("03c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5");
    ASSERT_EQ(expected_pub_key_two, pub_key_two);
}

// Test Checkpoint Serialization and Restoration
TEST(CheckpointTest, SaveAndLoadCheckpoint) {
    // Create a temporary directory for checkpoints
    std::string test_checkpoint_dir = "test_checkpoints";
    std::filesystem::create_directories(test_checkpoint_dir);

    Types::UInt256 lower(1000);
    Types::UInt256 upper(2000);
    Checkpoint::CheckpointManager mgr(test_checkpoint_dir, lower, upper);

    Progress::ScanStats stats;
    stats.keys_processed_total = 500;
    stats.current_position = 1500;
    stats.start_time = std::chrono::steady_clock::now() - std::chrono::seconds(100);

    std::vector<Checkpoint::WorkerCheckpointState> worker_states(Config::DEFAULT_WORKER_THREADS);
    worker_states[0].current_private_key_value = Types::UInt256(1200);
    worker_states[1].current_private_key_value = Types::UInt256(1400);

    mgr.save_checkpoint(stats, worker_states);

    // Load checkpoint into new stats and worker_states objects
    Progress::ScanStats loaded_stats;
    std::vector<Checkpoint::WorkerCheckpointState> loaded_worker_states;
    ASSERT_TRUE(mgr.load_latest_checkpoint(loaded_stats, loaded_worker_states));

    ASSERT_EQ(stats.keys_processed_total, loaded_stats.keys_processed_total);
    ASSERT_EQ(stats.current_position, loaded_stats.current_position);
    // Check elapsed time (might have slight difference due to system clock)
    ASSERT_NEAR(static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - loaded_stats.start_time).count()), 100.0, 2.0);

    ASSERT_EQ(worker_states.size(), loaded_worker_states.size());
    ASSERT_EQ(worker_states[0].current_private_key_value, loaded_worker_states[0].current_private_key_value);
    ASSERT_EQ(worker_states[1].current_private_key_value, loaded_worker_states[1].current_private_key_value);

    // Clean up temporary directory
    std::filesystem::remove_all(test_checkpoint_dir);
}

// Test Address Verification (using the target address as a known good example)
TEST(ScannerEngineTest, AddressVerification) {
    Types::Hash160 target_hash160 = hex_to_hash160(Config::TARGET_HASH160_HEX);

    // Create a dummy ScannerEngine to access hash160_to_address
    Scanner::ScannerEngine engine(Types::UInt256(0), Types::UInt256(1), target_hash160, 1);

    std::string derived_address = engine.hash160_to_address(target_hash160);
    ASSERT_EQ(Config::TARGET_ADDRESS, derived_address);
}

// Test Progress Calculations (basic check)
TEST(ProgressManagerTest, ProgressCalculations) {
    Types::UInt256 lower(0);
    Types::UInt256 upper(1000000);
    Progress::ProgressManager pm(lower, upper);
    pm.start_scan();

    pm.update_progress(100000, Types::UInt256(100000));
    // In a real scenario, time would pass, but for unit test, we check values directly
    ASSERT_EQ(pm.get_stats().keys_processed_total, Types::UInt256(100000));
    ASSERT_EQ(pm.get_stats().current_position, Types::UInt256(100000));

    pm.update_progress(200000, Types::UInt256(300000));
    ASSERT_EQ(pm.get_stats().keys_processed_total, Types::UInt256(300000));
    ASSERT_EQ(pm.get_stats().current_position, Types::UInt256(300000));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
