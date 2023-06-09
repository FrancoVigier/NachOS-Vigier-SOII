/// Routines for synchronizing threads.
///
/// The implementation for this primitive does not come with base Nachos.
/// It is left to the student.
///
/// When implementing this module, keep in mind that any implementation of a
/// synchronization routine needs some primitive atomic operation.  The
/// semaphore implementation, for example, disables interrupts in order to
/// achieve this; another way could be leveraging an already existing
/// primitive.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "lock.hh"
#include "semaphore.hh"
#include "system.hh"
#include <cstdio>
/// Dummy functions -- so we can compile our later assignments.

Lock::Lock(const char *debugName)
{
	name = debugName;
	semaphore = new Semaphore("Lock Semaphore", 1);
	lockOwner = nullptr;
}

Lock::~Lock()
{
	delete semaphore;
}

const char *
Lock::GetName() const
{
    return name;
}

void
Lock::Acquire() //The lockOwner is a thread who calls mutex
{
	ASSERT(!IsHeldByCurrentThread()); //If another thread call mutex, this thread sleep zzz... with semaphore->P()
	if(lockOwner != nullptr) {
		lockOwner->SetInheritedPriority(this, currentThread->GetPriority());
	}
	semaphore->P();
	lockOwner = currentThread;
}

void
Lock::Release() //Only the treadKey can unlock de mutex
{
    ASSERT(IsHeldByCurrentThread()); //Only the thread who call mutex can unlock them
	lockOwner->RemoveInheritedPriority(this);
	lockOwner = nullptr;
	semaphore->V();
}

bool
Lock::IsHeldByCurrentThread() const
{
    return lockOwner == currentThread;
}
