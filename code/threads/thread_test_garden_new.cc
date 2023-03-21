/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_garden_new.hh"
#include "semaphore.hh"
#include "system.hh"

#include <stdio.h>

struct Arguments  {
	unsigned n;
	Semaphore* semaphore;
};

static const unsigned NUM_TURNSTILES = 5;
static const unsigned ITERATIONS_PER_TURNSTILE = 50;
static bool done[NUM_TURNSTILES];
static int count;

static void
Turnstile(void *args)
{
	Arguments *arguments = (Arguments *) args;

    for (unsigned i = 0; i < ITERATIONS_PER_TURNSTILE; i++) {
	    arguments->semaphore->P();
	    int temp = count;
	    currentThread->Yield();
	    count = temp + 1;
	    arguments->semaphore->V();
    }
    printf("Turnstile %u finished. Count is now %u.\n", arguments->n, count);
    done[arguments->n] = true;
    delete arguments;
}

void
ThreadTestGardenNew()
{
	Semaphore* semaphore = new Semaphore("test semaphore", 3);

    // Launch a new thread for each turnstile.
    for (unsigned i = 0; i < NUM_TURNSTILES; i++) {
        printf("Launching turnstile %u.\n", i);
        char *name = new char [16];
        sprintf(name, "Turnstile %u", i);
        Thread *t = new Thread(name,false);
	    Arguments *arguments = new Arguments;
	    arguments->n = i;
		arguments->semaphore = semaphore;
        t->Fork(Turnstile, (void *) arguments);
    }

    // Wait until all turnstile threads finish their work.  `Thread::Join` is
    // not implemented at the beginning, therefore an ad-hoc workaround is
    // applied here.
    for (unsigned i = 0; i < NUM_TURNSTILES; i++) {
        while (!done[i]) {
            currentThread->Yield();
        }
    }
    printf("All turnstiles finished. Final count is %u (should be %u).\n",
           count, ITERATIONS_PER_TURNSTILE * NUM_TURNSTILES);
}
