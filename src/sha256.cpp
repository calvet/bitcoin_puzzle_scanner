#include "sha256.h"
#include <string.h>
#include "ripemd160.h"

namespace Hashing {

    // Rotate right
    static inline uint32_t rotr(uint32_t x, int n) {
        return (x >> n) | (x << (32 - n));
    }

    static inline uint32_t step1(uint32_t e, uint32_t f, uint32_t g) {
        return (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + ((e & f) ^ ((~e) & g));
    }

    static inline uint32_t step2(uint32_t a, uint32_t b, uint32_t c) {
        return (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
    }

    static void sha256_block(sha256_context *sha) {
        // State of the program
        uint32_t *state = sha->state;

        // Declare the K constant
        static const uint32_t k[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
            0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
            0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
            0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
            0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
            0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
            0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
            0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
            0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
        };

        uint32_t a = state[0];
        uint32_t b = state[1];
        uint32_t c = state[2];
        uint32_t d = state[3];
        uint32_t e = state[4];
        uint32_t f = state[5];
        uint32_t g = state[6];
        uint32_t h = state[7];

        uint32_t w[64];

        int j;
        // Initialize first 16 words from buffer
        for (j = 0; j < 16; j++) {
            w[j] =
                ((uint32_t)sha->buffer[j * 4 + 0] << 24) |
                ((uint32_t)sha->buffer[j * 4 + 1] << 16) |
                ((uint32_t)sha->buffer[j * 4 + 2] << 8) |
                ((uint32_t)sha->buffer[j * 4 + 3]);
        }

        // Extend to 64 words
        for (j = 16; j < 64; j++) {
            uint32_t s0 = (rotr(w[j - 15], 7) ^ rotr(w[j - 15], 18) ^ (w[j - 15] >> 3));
            uint32_t s1 = (rotr(w[j - 2], 17) ^ rotr(w[j - 2], 19) ^ (w[j - 2] >> 10));
            w[j] = w[j - 16] + s0 + w[j - 7] + s1;
        }

        for (j = 0; j < 64; j++) {
            uint32_t temp1 = h + step1(e, f, g) + k[j] + w[j];
            uint32_t temp2 = step2(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

    void sha256_init(sha256_context *sha) {
        sha->state[0] = 0x6a09e667;
        sha->state[1] = 0xbb67ae85;
        sha->state[2] = 0x3c6ef372;
        sha->state[3] = 0xa54ff53a;
        sha->state[4] = 0x510e527f;
        sha->state[5] = 0x9b05688c;
        sha->state[6] = 0x1f83d9ab;
        sha->state[7] = 0x5be0cd19;
        sha->n_bits = 0;
        sha->buffer_counter = 0;
    }

    void sha256_append_byte(sha256_context *sha, uint8_t byte) {
        sha->buffer[sha->buffer_counter++] = byte;
        sha->n_bits += 8;

        if (sha->buffer_counter == 64) {
            sha->buffer_counter = 0;
            sha256_block(sha);
        }
    }

    void sha256_append(sha256_context *sha, const void *src, size_t n_bytes) {
        const uint8_t *bytes = (const uint8_t*)src;
        size_t i;

        for (i = 0; i < n_bytes; i++) {
            sha256_append_byte(sha, bytes[i]);
        }
    }

    void sha256_finalize(sha256_context *sha) {
        int i;
        uint64_t n_bits = sha->n_bits;

        sha256_append_byte(sha, 0x80);

        while (sha->buffer_counter != 56) {
            sha256_append_byte(sha, 0);
        }

        for (i = 7; i >= 0; i--) {
            uint8_t byte = (n_bits >> (8 * i)) & 0xff;
            sha256_append_byte(sha, byte);
        }
    }

    void sha256_finalize_hex(sha256_context *sha, char *dst_hex65) {
        int i, j;
        sha256_finalize(sha);

        for (i = 0; i < 8; i++) {
            for (j = 7; j >= 0; j--) {
                uint8_t nibble = (sha->state[i] >> (j * 4)) & 0xf;
                *dst_hex65++ = "0123456789abcdef"[nibble];
            }
        }

        *dst_hex65 = '\0';
    }

    void sha256_finalize_bytes(sha256_context *sha, uint8_t *dst_bytes32) {
        uint8_t *ptr = (uint8_t*)dst_bytes32;
        int i;
        sha256_finalize(sha);

        for (i = 0; i < 8; i++) {
            // SHA256 output is big-endian
            *ptr++ = (sha->state[i] >> 24) & 0xff;
            *ptr++ = (sha->state[i] >> 16) & 0xff;
            *ptr++ = (sha->state[i] >> 8) & 0xff;
            *ptr++ = (sha->state[i] >> 0) & 0xff;
        }
    }

    void sha256_hex(const void *src, size_t n_bytes, char *dst_hex65) {
        sha256_context sha;
        sha256_init(&sha);
        sha256_append(&sha, src, n_bytes);
        sha256_finalize_hex(&sha, dst_hex65);
    }

    void sha256_bytes(const void *src, size_t n_bytes, uint8_t *dst_bytes32) {
        sha256_context sha;
        sha256_init(&sha);
        sha256_append(&sha, src, n_bytes);
        sha256_finalize_bytes(&sha, dst_bytes32);
    }

    void hash160(const void *src, size_t n_bytes, uint8_t *dst_hash160) {
        uint8_t sha256_result[SHA256_BYTES_SIZE];
        sha256_bytes(src, n_bytes, sha256_result);
        ripemd160(sha256_result, SHA256_BYTES_SIZE, dst_hash160);
    }

}
