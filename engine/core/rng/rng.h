#ifndef RNG_H
#define RNG_H

#include <stdint.h>

// [Source]: https://en.wikipedia.org/wiki/Xorshift

// Xorshift random number generator
uint32_t rng_generate(void);

// Generate a random number in a given range
uint32_t rng_range(uint32_t min, uint32_t max);

#endif