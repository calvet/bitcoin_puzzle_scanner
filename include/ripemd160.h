#ifndef BITCOIN_PUZZLE_SCANNER_RIPEMD160_H
#define BITCOIN_PUZZLE_SCANNER_RIPEMD160_H

#include <cstdint>
#include <stddef.h>

namespace Hashing {

    void ripemd160(const uint8_t* msg, uint32_t msg_len, uint8_t* hash);

}

#endif // BITCOIN_PUZZLE_SCANNER_RIPEMD160_H
