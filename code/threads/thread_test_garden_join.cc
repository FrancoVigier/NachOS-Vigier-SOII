/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_garden_join.hh"
#include "semaphore.hh"
#include "system.hh"
#include "lib/list.hh"

#include <stdio.h>

static const unsigned NUM_TURNSTILES = 5;
static const unsigned ITERATIONS_PER_TURNSTILE = 50;
static int count;

static void
Turnstile(void* args)
{
	int* arguments = (int*) args;

    for (unsigned i = 0; i < ITERATIONS_PER_TURNSTILE; i++) {
	    int temp = count;
	    currentThread->Yield();
	    count = temp + 1;
    }
    printf("Turnstile %i finished. Count is now %u.\n",*arguments, count);

}

void
ThreadTestGardenJoin()
{
    // Launch a new thread for each turnstile.
    for (unsigned i = 0; i < NUM_TURNSTILES; i++) {
        printf("Launching turnstile %u.\n", i);
        char *name = new char [16];
        sprintf(name, "Turnstile %u", i);
        Thread *t = new Thread(name,true);
        int* m = (int*)malloc(sizeof (int));
        *m=i;
        t->Fork(Turnstile,(void* ) m);
        printf("El main en espera del Turnstile %i\n",i);
        t->Join();
        printf("El main garantiza Turnstile %i Join\n",i);
    }


    printf("All turnstiles joined. Final count is %u (should be %u).\n",
           count, ITERATIONS_PER_TURNSTILE * NUM_TURNSTILES);
}
