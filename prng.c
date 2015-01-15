/*
 * Mersenne Twister (MT19937) PRNG.
 */

#include <string.h>
#include "prng.h"


void prng_init(struct prng *prng, uint32_t seed)
{
	uint32_t y;

	prng->index = 0;
	prng->state[0] = seed;
	for (uint32_t i = 1; i < 624; i++) {
		y = prng->state[i - 1] ^ (prng->state[i - 1] >> 30);
		prng->state[i] = UINT32_C(0x6c078965) * y + i;
	}
}

static void mt_regen(struct prng *prng)
{
	uint32_t y;

	for (uint32_t i = 0; i < 624; i++) {
		y = ((prng->state[i] & UINT32_C(0x80000000)) |
		     (prng->state[(i + 1) % 624] & UINT32_C(0x7fffffff)));
		prng->state[i] = prng->state[(i + 397) % 624] ^ (y >> 1);
		if (y % 2)
			prng->state[i] ^= UINT32_C(0x9908b0df);
	}
}

static uint32_t mt_word(struct prng *prng)
{
	uint32_t y;

	if (prng->index == 0)
		mt_regen(prng);

	y = prng->state[prng->index];
	y ^= y >> 11;
	y ^= (y << 7) & UINT32_C(0x9d2c5680);
	y ^= (y << 15) & UINT32_C(0xefc60000);
	y ^= y >> 18;

	prng->index = (prng->index + 1) % 624;
	return y;
}

uint32_t prng_range(struct prng *prng, uint32_t low, uint32_t high)
{
	/* Not actually perfectly uniform... Oh well. */
	return (mt_word(prng) % (high - low)) + low;
}

bool prng_bool(struct prng *prng, double true_false_ratio)
{
	return (double)mt_word(prng) <= (double)UINT32_MAX * true_false_ratio;
}

void prng_bytes(struct prng *prng, char *buf, size_t count)
{
	uint32_t y;

	while (count >= sizeof(uint32_t)) {
		y = mt_word(prng);
		memcpy(buf, &y, sizeof(uint32_t));
		buf += sizeof(uint32_t);
		count -= sizeof(uint32_t);
	}

	if (count) {
		y = mt_word(prng);
		memcpy(buf, &y, count);
	}
}
