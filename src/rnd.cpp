#include "rnd.hpp"
#include <time.h>

#define RND_A 12733u
#define RND_C 3727465369u

static uint32_t seed = 123;

void rnd_start(uint32_t s)
{
	if(s <= 3)
		s = time(NULL);
	seed = s;
}

uint32_t rnd_next(uint32_t x)
{
	return x * RND_A + RND_C;
}

uint32_t rnd()
{
	seed = seed * RND_A + RND_C;
	return seed;
}

uint32_t rnd(uint32_t d)
{
	uint64_t x = rnd();
	x *= d;
	x >>= 32;
	return x;
}

float rnd(float d)
{
	rnd();
	float x = seed;
	x *= d;
	x /= (float)(1ULL << 32);
	return x;
}
