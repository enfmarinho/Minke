/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef HASH_H
#define HASH_H

#include <cassert>

#include "types.h"

// Randoms uint64_t for performing Zobrist Hashing.
struct HashKeys {
    HashType pieces[12][64];
    HashType castle[16];
    HashType en_passant[8];
    HashType side;
};
extern HashKeys hash_keys;

//==== Taken from stockfish
// xorshift64star Pseudo-Random Number Generator
// This class is based on original code written and dedicated
// to the public domain by Sebastiano Vigna (2014).
// It has the following characteristics:
//
//  -  Outputs 64-bit numbers
//  -  Passes Dieharder and SmallCrush test batteries
//  -  Does not require warm-up, no zeroland to escape
//  -  Internal state is a single 64-bit integer
//  -  Period is 2^64 - 1
//  -  Speed: 1.60 ns/call (Core i7 @3.40GHz)
//
// For further analysis see
//   <http://vigna.di.unimi.it/ftp/papers/xorshift.pdf>

class PRNG {
    HashType s;

    HashType rand64() {
        s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
        return s * 2685821657736338717LL;
    }

  public:
    PRNG(HashType seed) : s(seed) { assert(seed); }

    template <typename T>
    T rand() {
        return T(rand64());
    }

    // Special generator used to fast init magic numbers.
    // Output values only have 1/8th of their bits set on average.
    template <typename T>
    T sparse_rand() {
        return T(rand64() & rand64() & rand64());
    }
};
extern PRNG prng;

#endif // #ifndef HASH_H
