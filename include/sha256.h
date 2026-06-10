#ifndef BITCOIN_PUZZLE_SCANNER_SHA256_H
#define BITCOIN_PUZZLE_SCANNER_SHA256_H

#include <stddef.h>
#include <stdint.h>
#include <array>

namespace Hashing {

#define SHA256_HEX_SIZE (64 + 1)
#define SHA256_BYTES_SIZE 32

    typedef struct sha256_context {
        uint32_t state[8];
        uint8_t buffer[64];
        uint64_t n_bits;
        uint8_t buffer_counter;
    } sha256_context;

    void sha256_init(sha256_context *sha);
    void sha256_append(sha256_context *sha, const void *data, size_t n_bytes);
    void sha256_finalize_hex(sha256_context *sha, char *dst_hex65);
    void sha256_finalize_bytes(sha256_context *sha, uint8_t *dst_bytes32);

    // Convenience functions
    void sha256_hex(const void *src, size_t n_bytes, char *dst_hex65);
    void sha256_bytes(const void *src, size_t n_bytes, uint8_t *dst_bytes32);

    // Function to compute HASH160 (SHA256 then RIPEMD160)
    // This will require a ripemd160 implementation as well.
    // For now, just declare the interface.
    void hash160(const void *src, size_t n_bytes, uint8_t *dst_hash160);

}

#endif // BITCOIN_PUZZLE_SCANNER_SHA256_H
