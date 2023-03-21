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


#define SPACE (100*1024)
unsigned char MEMORY[SPACE]; //100kB

int
main(void)
{
  for (int i = 0; i < SPACE; i += 128) { // 128 is the page size
    MEMORY[i] = i;
  }
}
