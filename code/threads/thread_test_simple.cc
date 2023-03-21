/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_simple.hh"
#include "semaphore.hh"
#include "system.hh"

#include <stdio.h>
#include <string.h>

struct Arguments  {
	char name[64];
	Semaphore* semaphore;
};


/// Loop 10 times, yielding the CPU to another ready thread each iteration.
///
/// * `name` points to a string with a thread name, just for debugging
///   purposes.
static void
SimpleThread(void *name_)
{
	// Reinterpret arg `name` as a string.
	Arguments *arguments = (Arguments *) name_;

	// If the lines dealing with interrupts are commented, the code will
	// behave incorrectly, because printf execution may cause race
	// conditions.
	for (unsigned num = 0; num < 10; num++) {
		if(arguments->semaphore) {
			DEBUG('t', "Semaphore was decremented\n");
			arguments->semaphore->P();
		}

		printf("*** Thread `%s` is running: iteration %u\n", arguments->name, num);

		if(arguments->semaphore) {
			DEBUG('t', "Semaphore was incremented\n");
			arguments->semaphore->V();
		}

		currentThread->Yield();
	}
	printf("!!! Thread `%s` has finished\n", arguments->name);
}

static Arguments*
CreateThread(const char * n, Semaphore* semaphore) {
	Arguments* arguments = new Arguments;
	arguments->semaphore = semaphore;
	strncpy(arguments->name, n, 64);
	Thread *newThread = new Thread(arguments->name,false);
	newThread->Fork(SimpleThread, (void *) arguments);
	return arguments;
}

/// Set up a ping-pong between several threads.
///
/// Do it by launching one thread which calls `SimpleThread`, and finally
/// calling `SimpleThread` on the current thread.
void
ThreadTestSimple()
{
#ifdef SEMAPHORE_TEST
	Semaphore* semaphore = new Semaphore("test semaphore", 3);
#else
	Semaphore* semaphore = nullptr;
#endif

	Arguments* args2 = CreateThread("2nd", semaphore);
	Arguments* args3 = CreateThread("3rd", semaphore);
	Arguments* args4 = CreateThread("4th", semaphore);
	Arguments* args5 = CreateThread("5th", semaphore);

	Arguments arguments = { "1st", semaphore };
	SimpleThread((void *) &arguments);

	delete semaphore;
	delete args2;
	delete args3;
	delete args4;
	delete args5;
}
