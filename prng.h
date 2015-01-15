/*
 * Monte Carlo PRNG.
 */

#ifndef PRNG_H
#define PRNG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct prng {
	uint32_t state[624];
	int index;
};

/*
 * prng_init - initialize a PRNG
 * @prng: the PRNG to initialize
 * @seed: the seed; two sequences with the same seed are guaranteed to produce
 * the same result
 */
void prng_init(struct prng *prng, uint32_t seed);

/**
 * prng_range - generate a random number in a uniformly distributed range
 * @prng: the PRNG
 * @low: inclusive lower bound
 * @high: exclusive upper bound
 */
uint32_t prng_range(struct prng *prng, uint32_t low, uint32_t high);

/**
 * prng_bool - generate a random boolean
 * @prng: the PRNG
 * @true_false_ratio: ratio of true to false (e.g., 0.8 means 80% true)
 */
bool prng_bool(struct prng *prng, double true_false_ratio);

/**
 * prng_bytes - generate random bytes
 * @prng: the PRNG
 * @buf: buffer to write the bytes to
 * @count: number of bytes to write
 */
void prng_bytes(struct prng *prng, char *buf, size_t count);

#endif /* PRNG_H */
