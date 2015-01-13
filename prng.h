/*
 * Monte Carlo PRNG.
 */

#ifndef PRNG_H
#define PRNG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * prng_seed - seed the PRNG; two sequences with the same seed are guaranteed to
 * produce the same result
 * @seed: the seed
 */
void prng_seed(uint32_t seed);

/**
 * prng_range - generate a random number in a uniformly distributed range
 * @low: inclusive lower bound
 * @high: exclusive upper bound
 */
uint32_t prng_range(uint32_t low, uint32_t high);

/**
 * prng_bool - generate a random boolean
 * @true_false_ratio: ratio of true to false (e.g., 0.8 means 80% true)
 */
bool prng_bool(double true_false_ratio);

/**
 * prng_bytes - generate random bytes
 * @buf: buffer to write the bytes to
 * @count: number of bytes to write
 */
void prng_bytes(char *buf, size_t count);

#endif /* PRNG_H */
