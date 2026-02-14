/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef HASH_H
#define HASH_H

#include <cassert>
#include <limits>
#include <random>

#include "types.h"

// Randoms uint64_t for performing Zobrist Hashing.
struct HashKeys {
    HashType pieces[12][64];
    HashType castle[16];
    HashType en_passant[8];
    HashType side;
};
extern HashKeys hash_keys;

// Implements the Splitmix64 algorithm to generate seeds for the xorshift64star PRNG
class SeedGenerator {
  public:
    static uint64_t master_seed() {
        // This is not portable considering that random_device may be pseudo-random depending on hardware
        // but this shouldn't be a problem
        std::random_device rd{};
        return static_cast<uint64_t>(rd()) << 32 | static_cast<uint64_t>(rd());
    }

    SeedGenerator() = delete;
    SeedGenerator(uint64_t seed = master_seed()) : m_state(seed) {}

    uint64_t next() {
        m_state += 0x9e3779b97f4a7c15ULL;

        uint64_t z = m_state;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }

  private:
    uint64_t m_state;
};

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
  public:
    using result_type = uint64_t;

    PRNG(result_type seed) : s(seed) { assert(seed); }

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

    result_type operator()() { return rand64(); }
    static constexpr result_type min() { return std::numeric_limits<result_type>::min(); }
    static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }

  public:
    result_type s;

    result_type rand64() {
        s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
        return s * 2685821657736338717LL;
    }
};

#endif // #ifndef HASH_H
