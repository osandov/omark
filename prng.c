/*
 * Mersenne Twister (MT19937) PRNG.
 */

#include <string.h>
#include "prng.h"

static uint32_t mt[624];
static int mt_index;

void prng_seed(uint32_t seed)
{
	uint32_t y;

	mt_index = 0;
	mt[0] = seed;
	for (uint32_t i = 1; i < 624; i++) {
		y = mt[i - 1] ^ (mt[i - 1] >> 30);
		mt[i] = UINT32_C(0x6c078965) * y + i;
	}
}

static void mt_regen(void)
{
	uint32_t y;

	for (uint32_t i = 0; i < 624; i++) {
		y = ((mt[i] & UINT32_C(0x80000000)) |
		     (mt[(i + 1) % 624] & UINT32_C(0x7fffffff)));
		mt[i] = mt[(i + 397) % 624] ^ (y >> 1);
		if (y % 2)
			mt[i] ^= UINT32_C(0x9908b0df);
	}
}

static uint32_t mt_word(void)
{
	uint32_t y;

	if (mt_index == 0)
		mt_regen();

	y = mt[mt_index];
	y ^= y >> 11;
	y ^= (y << 7) & UINT32_C(0x9d2c5680);
	y ^= (y << 15) & UINT32_C(0xefc60000);
	y ^= y >> 18;

	mt_index = (mt_index + 1) % 624;
	return y;
}

uint32_t prng_range(uint32_t low, uint32_t high)
{
	/* Not actually perfectly uniform... Oh well. */
	return (mt_word() % (high - low)) + low;
}

bool prng_bool(double true_false_ratio)
{
	return (double)mt_word() <= (double)UINT32_MAX * true_false_ratio;
}

void prng_bytes(char *buf, size_t count)
{
	uint32_t y;

	while (count >= sizeof(uint32_t)) {
		y = mt_word();
		memcpy(buf, &y, sizeof(uint32_t));
		buf += sizeof(uint32_t);
		count -= sizeof(uint32_t);
	}

	if (count) {
		y = mt_word();
		memcpy(buf, &y, count);
	}
}
