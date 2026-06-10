/*
 * Copyright (c) 2015 Jonas Schnelli
 * Copyright (c) 2013-2014 Tomas Dzetkulic
 * Copyright (c) 2013-2014 Pavol Rusnak
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ripemd160.h"
#include <string.h>

namespace Hashing {

#define ROL(x, n) ((x << n) | (x >> (32 - n)))

static void compress(uint32_t *digest, uint32_t *X)
{
    uint32_t a = digest[0], b = digest[1], c = digest[2], d = digest[3], e = digest[4];
    uint32_t aa, bb, cc, dd, ee;

    aa = a; bb = b; cc = c; dd = d; ee = e;

    /* Round 1 */
#define F1(x, y, z) (x ^ y ^ z)
#define P1(a, b, c, d, e, x, s) { a += F1(b, c, d) + x; a = ROL(a, s) + e; c = ROL(c, 10); }
    P1(a, b, c, d, e, X[ 0], 11); P1(e, a, b, c, d, X[ 1], 14); P1(d, e, a, b, c, X[ 2], 15); P1(c, d, e, a, b, X[ 3], 12);
    P1(b, c, d, e, a, X[ 4],  5); P1(a, b, c, d, e, X[ 5],  8); P1(e, a, b, c, d, X[ 6],  7); P1(d, e, a, b, c, X[ 7],  9);
    P1(c, d, e, a, b, X[ 8], 11); P1(b, c, d, e, a, X[ 9], 13); P1(a, b, c, d, e, X[10], 14); P1(e, a, b, c, d, X[11], 15);
    P1(d, e, a, b, c, X[12],  6); P1(c, d, e, a, b, X[13],  7); P1(b, c, d, e, a, X[14],  9); P1(a, b, c, d, e, X[15],  8);
#undef P1
#undef F1

    /* Round 2 */
#define F2(x, y, z) ((x & y) | (~x & z))
#define P2(a, b, c, d, e, x, s) { a += F2(b, c, d) + x + 0x5a827999UL; a = ROL(a, s) + e; c = ROL(c, 10); }
    P2(a, b, c, d, e, X[ 7],  7); P2(e, a, b, c, d, X[ 4],  6); P2(d, e, a, b, c, X[13],  8); P2(c, d, e, a, b, X[ 1], 13);
    P2(b, c, d, e, a, X[10], 11); P2(a, b, c, d, e, X[ 6],  9); P2(e, a, b, c, d, X[15],  7); P2(d, e, a, b, c, X[ 3], 15);
    P2(c, d, e, a, b, X[12],  7); P2(b, c, d, e, a, X[ 0], 12); P2(a, b, c, d, e, X[ 9], 15); P2(e, a, b, c, d, X[ 5],  9);
    P2(d, e, a, b, c, X[ 2],  8); P2(c, d, e, a, b, X[14], 11); P2(b, c, d, e, a, X[11], 13); P2(a, b, c, d, e, X[ 8], 14);
#undef P2
#undef F2

    /* Round 3 */
#define F3(x, y, z) ((x | ~y) ^ z)
#define P3(a, b, c, d, e, x, s) { a += F3(b, c, d) + x + 0x6ed9eba1UL; a = ROL(a, s) + e; c = ROL(c, 10); }
    P3(a, b, c, d, e, X[ 3], 11); P3(e, a, b, c, d, X[10], 13); P3(d, e, a, b, c, X[14],  6); P3(c, d, e, a, b, X[ 4],  7);
    P3(b, c, d, e, a, X[ 9], 14); P3(a, b, c, d, e, X[15],  9); P3(e, a, b, c, d, X[ 8], 13); P3(d, e, a, b, c, X[ 1], 15);
    P3(c, d, e, a, b, X[ 2], 14); P3(b, c, d, e, a, X[ 7],  8); P3(a, b, c, d, e, X[ 0], 13); P3(e, a, b, c, d, X[ 6],  6);
    P3(d, e, a, b, c, X[13],  5); P3(c, d, e, a, b, X[11], 12); P3(b, c, d, e, a, X[ 5],  7); P3(a, b, c, d, e, X[12],  5);
#undef P3
#undef F3

    /* Round 4 */
#define F4(x, y, z) ((x & z) | (y & ~z))
#define P4(a, b, c, d, e, x, s) { a += F4(b, c, d) + x + 0x8f1bbcdcUL; a = ROL(a, s) + e; c = ROL(c, 10); }
    P4(a, b, c, d, e, X[ 1], 11); P4(e, a, b, c, d, X[ 9], 12); P4(d, e, a, b, c, X[11], 14); P4(c, d, e, a, b, X[10], 15);
    P4(b, c, d, e, a, X[ 0], 14); P4(a, b, c, d, e, X[ 8], 15); P4(e, a, b, c, d, X[12],  9); P4(d, e, a, b, c, X[ 4],  8);
    P4(c, d, e, a, b, X[13],  9); P4(b, c, d, e, a, X[ 3], 14); P4(a, b, c, d, e, X[ 7],  5); P4(e, a, b, c, d, X[15],  6);
    P4(d, e, a, b, c, X[14],  8); P4(c, d, e, a, b, X[ 5],  5); P4(b, c, d, e, a, X[ 6], 12); P4(a, b, c, d, e, X[ 2], 13);
#undef P4
#undef F4

    /* Round 5 */
#define F5(x, y, z) (x ^ (y | ~z))
#define P5(a, b, c, d, e, x, s) { a += F5(b, c, d) + x + 0xa953fd4eUL; a = ROL(a, s) + e; c = ROL(c, 10); }
    P5(a, b, c, d, e, X[ 4],  9); P5(e, a, b, c, d, X[ 0], 15); P5(d, e, a, b, c, X[ 5],  5); P5(c, d, e, a, b, X[ 9], 11);
    P5(b, c, d, e, a, X[ 7],  6); P5(a, b, c, d, e, X[12],  8); P5(e, a, b, c, d, X[ 2], 13); P5(d, e, a, b, c, X[10], 12);
    P5(c, d, e, a, b, X[14],  5); P5(b, c, d, e, a, X[ 1], 12); P5(a, b, c, d, e, X[ 3], 13); P5(e, a, b, c, d, X[ 8], 14);
    P5(d, e, a, b, c, X[11], 11); P5(c, d, e, a, b, X[ 6],  8); P5(b, c, d, e, a, X[15],  5); P5(a, b, c, d, e, X[13],  6);
#undef P5
#undef F5

    aa = aa + c + digest[1];
    bb = bb + d + digest[2];
    cc = cc + e + digest[3];
    dd = dd + a + digest[4];
    ee = ee + b + digest[0];

    digest[0] = aa; digest[1] = bb; digest[2] = cc; digest[3] = dd; digest[4] = ee;

    /* Parallel round 1 */
#define F1(x, y, z) (x ^ y ^ z)
#define P1(a, b, c, d, e, x, s) { a += F1(b, c, d) + x + 0x50a28be6UL; a = ROL(a, s) + e; c = ROL(c, 10); }
    P1(a, b, c, d, e, X[ 5],  8); P1(e, a, b, c, d, X[14],  9); P1(d, e, a, b, c, X[ 7],  9); P1(c, d, e, a, b, X[ 0], 11);
    P1(b, c, d, e, a, X[ 9], 13); P1(a, b, c, d, e, X[ 2], 15); P1(e, a, b, c, d, X[11], 15); P1(d, e, a, b, c, X[ 4],  5);
    P1(c, d, e, a, b, X[13],  7); P1(b, c, d, e, a, X[ 6],  7); P1(a, b, c, d, e, X[15],  8); P1(e, a, b, c, d, X[ 8], 11);
    P1(d, e, a, b, c, X[ 1], 14); P1(c, d, e, a, b, X[10], 14); P1(b, c, d, e, a, X[ 3], 12); P1(a, b, c, d, e, X[12],  6);
#undef P1
#undef F1

    /* Parallel round 2 */
#define F2(x, y, z) ((x & z) | (y & ~z))
#define P2(a, b, c, d, e, x, s) { a += F2(b, c, d) + x + 0x5cdd1246UL; a = ROL(a, s) + e; c = ROL(c, 10); }
    P2(a, b, c, d, e, X[ 6],  9); P2(e, a, b, c, d, X[11], 13); P2(d, e, a, b, c, X[ 3], 15); P2(c, d, e, a, b, X[ 7],  7);
    P2(b, c, d, e, a, X[ 0], 12); P2(a, b, c, d, e, X[13],  8); P2(e, a, b, c, d, X[ 5],  9); P2(d, e, a, b, c, X[10], 11);
    P2(c, d, e, a, b, X[14],  7); P2(b, c, d, e, a, X[15],  7); P2(a, b, c, d, e, X[ 8], 12); P2(e, a, b, c, d, X[12], 15);
    P2(d, e, a, b, c, X[ 4],  9); P2(c, d, e, a, b, X[ 1], 11); P2(b, c, d, e, a, X[ 2], 13); P2(a, b, c, d, e, X[ 9], 11);
#undef P2
#undef F2

    /* Parallel round 3 */
#define F3(x, y, z) ((x | ~y) ^ z)
#define P3(a, b, c, d, e, x, s) { a += F3(b, c, d) + x + 0x6d703ef3UL; a = ROL(a, s) + e; c = ROL(c, 10); }
    P3(a, b, c, d, e, X[15],  9); P3(e, a, b, c, d, X[ 5],  7); P3(d, e, a, b, c, X[ 1], 15); P3(c, d, e, a, b, X[ 3], 11);
    P3(b, c, d, e, a, X[ 7],  8); P3(a, b, c, d, e, X[14],  7); P3(e, a, b, c, d, X[ 6], 11); P3(d, e, a, b, c, X[ 9], 13);
    P3(c, d, e, a, b, X[11], 11); P3(b, c, d, e, a, X[ 0], 12); P3(a, b, c, d, e, X[ 8],  5); P3(e, a, b, c, d, X[12],  6);
    P3(d, e, a, b, c, X[ 2],  7); P3(c, d, e, a, b, X[10], 13); P3(b, c, d, e, a, X[ 4], 11); P3(a, b, c, d, e, X[13], 12);
#undef P3
#undef F3

    /* Parallel round 4 */
#define F4(x, y, z) ((x & y) | (~x & z))
#define P4(a, b, c, d, e, x, s) { a += F4(b, c, d) + x + 0x7a6d76e9UL; a = ROL(a, s) + e; c = ROL(c, 10); }
    P4(a, b, c, d, e, X[ 8], 15); P4(e, a, b, c, d, X[ 6],  5); P4(d, e, a, b, c, X[ 4],  8); P4(c, d, e, a, b, X[ 1], 11);
    P4(b, c, d, e, a, X[ 3], 14); P4(a, b, c, d, e, X[11], 14); P4(e, a, b, c, d, X[15],  6); P4(d, e, a, b, c, X[ 0], 14);
    P4(c, d, e, a, b, X[ 5],  6); P4(b, c, d, e, a, X[12],  9); P4(a, b, c, d, e, X[ 2], 12); P4(e, a, b, c, d, X[13],  9);
    P4(d, e, a, b, c, X[ 9], 12); P4(c, d, e, a, b, X[ 7],  5); P4(b, c, d, e, a, X[10], 15); P4(a, b, c, d, e, X[14],  8);
#undef P4
#undef F4

    /* Parallel round 5 */
#define F5(x, y, z) (x ^ y ^ z)
#define P5(a, b, c, d, e, x, s) { a += F5(b, c, d) + x; a = ROL(a, s) + e; c = ROL(c, 10); }
    P5(a, b, c, d, e, X[12],  8); P5(e, a, b, c, d, X[15],  5); P5(d, e, a, b, c, X[10], 12); P5(c, d, e, a, b, X[ 4],  9);
    P5(b, c, d, e, a, X[ 1], 12); P5(a, b, c, d, e, X[ 7],  5); P5(e, a, b, c, d, X[ 9], 14); P5(d, e, a, b, c, X[ 3],  6);
    P5(c, d, e, a, b, X[13],  8); P5(b, c, d, e, a, X[14], 13); P5(a, b, c, d, e, X[ 0],  5); P5(e, a, b, c, d, X[ 6], 15);
    P5(d, e, a, b, c, X[ 2], 13); P5(c, d, e, a, b, X[11], 11); P5(b, c, d, e, a, X[ 8], 11); P5(a, b, c, d, e, X[ 5], 12);
#undef P5
#undef F5

    digest[0] += aa; digest[1] += bb; digest[2] += cc; digest[3] += dd; digest[4] += ee;
}

void ripemd160(const uint8_t* msg, uint32_t msg_len, uint8_t* hash)
{
    uint32_t i;
    int j;
    uint32_t digest[5] = {0x67452301UL, 0xefcdab89UL, 0x98badcfeUL, 0x10325476UL, 0xc3d2e1f0UL};

    for (i = 0; i < (msg_len >> 6); ++i) {
        uint32_t chunk[16];
        for (j = 0; j < 16; ++j) {
            chunk[j] =
                (uint32_t)(*(msg++));
            chunk[j] |= (uint32_t)(*(msg++)) << 8;
            chunk[j] |= (uint32_t)(*(msg++)) << 16;
            chunk[j] |= (uint32_t)(*(msg++)) << 24;
        }
        compress(digest, chunk);
    }

    // Last chunk
    {
        uint32_t chunk[16] = {0};
        for (i = 0; i < (msg_len & 63); ++i) {
            chunk[i >> 2] ^= (uint32_t)*msg++ << ((i & 3) << 3);
        }
        chunk[(msg_len >> 2) & 15] ^= (uint32_t)1 << (8 * (msg_len & 3) + 7);

        if ((msg_len & 63) > 55) {
            compress(digest, chunk);
            memset(chunk, 0, 64);
        }

        chunk[14] = msg_len << 3;
        chunk[15] = (msg_len >> 29);
        compress(digest, chunk);
    }

    for (i = 0; i < 5; ++i) {
        *(hash++) = (uint8_t)digest[i];
        *(hash++) = (uint8_t)(digest[i] >> 8);
        *(hash++) = (uint8_t)(digest[i] >> 16);
        *(hash++) = (uint8_t)(digest[i] >> 24);
    }
}

}
