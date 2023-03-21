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


#include "condition.hh"
#include "system.hh"
#include <cstdio>
/// Dummy functions -- so we can compile our later assignments.
///
/// Note -- without a correct implementation of `Condition::Wait`, the test
/// case in the network assignment will not work!

Condition::Condition(const char *debugName, Lock *conditionLock)
{
    name = debugName;
    cantidadDormidos = 0;
    lock = conditionLock;
    colaProcesosDormidos = new List <Semaphore *>;
}

Condition::~Condition()
{
    ///
    while(! colaProcesosDormidos->IsEmpty())
        delete colaProcesosDormidos->Pop();
    delete colaProcesosDormidos;
}

const char *
Condition::GetName() const
{
    return name;
}

void
Condition::Wait()
{
	ASSERT(lock->IsHeldByCurrentThread());
    Semaphore* aDormir = new Semaphore(currentThread->GetName(),0); //Semaphore with 0 capacity, if i use P the process go to sleep now
    colaProcesosDormidos->Append(aDormir); //I save the semaphore for a future process who call V in Signal Function, we save aDormir by reference in the queue
    lock->Release(); //Put off the lock, im going to sleep no?
    cantidadDormidos++;
    aDormir->P(); //Go to sleep NOW
    lock->Acquire(); //We try to get the lock again because we are active
}

void
Condition::Signal()
{
	ASSERT(lock->IsHeldByCurrentThread());
    if(!colaProcesosDormidos->IsEmpty()){
       Semaphore* procesoADespertar = colaProcesosDormidos->Pop();//We get any slipping process' semaphore
       cantidadDormidos--;
       procesoADespertar->V(); //Get up bro
	}
}

void
Condition::Broadcast()
{
//	ASSERT(lock->IsHeldByCurrentThread());
       while(!colaProcesosDormidos->IsEmpty())
        Signal();
}

int
Condition::SleepingThreadAmount()
{
	return cantidadDormidos;
}
