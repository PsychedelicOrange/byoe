#include "rng.h"

// Seed value for the RNG
static uint32_t xorshift32_state = 23122023; // You can set any non-zero seed value

uint32_t rng_generate(void) {
    uint32_t x = xorshift32_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    xorshift32_state = x;
    return x;
}

// Generate a random number in a given range
uint32_t rng_range(uint32_t min, uint32_t max) {
    return (rng_generate() % (max - min + 1)) + min;
}