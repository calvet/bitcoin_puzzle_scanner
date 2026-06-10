#ifndef BITCOIN_PUZZLE_SCANNER_TYPES_H
#define BITCOIN_PUZZLE_SCANNER_TYPES_H

#include <cstdint>
#include <vector>
#include <array>
#include "uint256.h"

namespace Types {
    // Define a 256-bit private key type (for future expansion, currently using uint64_t for puzzle 71 range)
    using PrivateKey = std::array<uint8_t, 32>;

    // Define a 33-byte compressed public key type
    using PublicKeyCompressed = std::array<uint8_t, 33>;

    // Define a 20-byte HASH160 type
    using Hash160 = std::array<uint8_t, 20>;

    // Define a 32-byte SHA256 hash type
    using Sha256 = std::array<uint8_t, 32>;

    // Define a 20-byte RIPEMD160 hash type
    using Ripemd160 = std::array<uint8_t, 20>;
}

#endif // BITCOIN_PUZZLE_SCANNER_TYPES_H
