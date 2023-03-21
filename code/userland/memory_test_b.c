/// Simple program to test whether running a user program works.
///
/// Just do a “syscall” that shuts down the OS.
///
/// NOTE: for some reason, user programs with global data structures
/// sometimes have not worked in the Nachos environment.  So be careful out
/// there!  One option is to allocate data structures as automatics within a
/// procedure, but if you do this, you have to be careful to allocate a big
/// enough stack to hold the automatics!

#include "syscall.h"

/* The Xorwow PRNG state. This must not be initialized to all zeros. */
static unsigned int  prng_state[5] = { 1, 2, 3, 4, 5 };

/* The Xorwow is a 32-bit linear-feedback shift generator. */
#define  PRNG_MAX  4294967295u

unsigned int  prng(void)
{
	unsigned int  s, t;

	t = prng_state[3] & PRNG_MAX;
	t ^= t >> 2;
	t ^= t << 1;
	prng_state[3] = prng_state[2];
	prng_state[2] = prng_state[1];
	prng_state[1] = prng_state[0];
	s = prng_state[0] & PRNG_MAX;
	t ^= s;
	t ^= (s << 4) & PRNG_MAX;
	prng_state[0] = t;
	prng_state[4] = (prng_state[4] + 362437) & PRNG_MAX;
	return (t + prng_state[4]) & PRNG_MAX;
}


#define SPACE (100*1024)
unsigned char MEMORY[SPACE]; //100kB

int
main(void)
{
  for (int i = 0; i < SPACE; ++i) {
    MEMORY[prng()%SPACE] = i;
  }
}
