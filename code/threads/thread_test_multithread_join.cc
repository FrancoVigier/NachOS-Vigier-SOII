/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_multithread_join.hh"
#include "semaphore.hh"
#include "system.hh"

#include <stdio.h>
#include <string.h>

static bool longOperationFinished = false;

struct Arguments {
	char name[64];
    Thread* toJoinThread;
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
	for(volatile int i = 0; i < 5000; i++) {}

	printf("Now joining\n");

	arguments->toJoinThread->Join();
	printf("!!! Thread \"%s\" has finished\n", arguments->name);
}

static void
longOperation(void* _) {
	for(volatile int i = 0; i < 500000000; i++) {
		// Long
	}

	longOperationFinished = true;
	printf("!!! Thread \"Long operation\" has finished\n");
}

static Arguments*
CreateThread(const char * n, Thread* thread, Thread **newThread) {
	Arguments* arguments = new Arguments;
	arguments->toJoinThread = thread;
	strncpy(arguments->name, n, 64);
	*newThread = new Thread(arguments->name, true, 1);
	(*newThread)->Fork(SimpleThread, (void *) arguments);
	return arguments;
}

/// Set up a ping-pong between several threads.
///
/// Do it by launching one thread which calls `SimpleThread`, and finally
/// calling `SimpleThread` on the current thread.
void
ThreadTestMutlithreadJoin()
{
	Thread *operation = new Thread("Long operation", true);
	operation->Fork(longOperation, nullptr);

	Thread* thread1;
	Arguments* args1 = CreateThread("1st", operation, &thread1);
	Thread* thread2;
	Arguments* args2 = CreateThread("2nd", operation, &thread2);
	Thread* thread3;
	Arguments* args3 = CreateThread("3rd", operation, &thread3);
	Thread* thread4;
	Arguments* args4 = CreateThread("4th", operation, &thread4);

	thread1->Join();
	ASSERT(longOperationFinished);
	thread2->Join();
	ASSERT(longOperationFinished);
	thread3->Join();
	ASSERT(longOperationFinished);
	thread4->Join();
	ASSERT(longOperationFinished);

	delete args1;
	delete args2;
	delete args3;
	delete args4;
}
