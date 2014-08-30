//#define DEBUG 1

#include <stdint.h>

void rnd_start(uint32_t);
uint32_t rnd(uint32_t);
uint32_t rnd();
uint32_t rnd_next(uint32_t x);
float rnd(float d);
#ifdef DEBUG
void ran_test();
#endif
