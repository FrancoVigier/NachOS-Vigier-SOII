/// Copyright (c) 2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_SYNCH_CONSOLE__HH
#define NACHOS_SYNCH_CONSOLE__HH


#include "threads/lock.hh"
#include "threads/semaphore.hh"
#include "machine/console.hh"


class SynchConsole {
public:
    SynchConsole(const char* in, const char* out);
    ~SynchConsole();

    char ReadCharFromConsole();
    void WriteCharToConsol(char c);

private:

    Lock* rLock;
    Semaphore* rSem;

    Lock* wLock;
    Semaphore* wSem;

    Console* console;
    static void WriteOK(void* data);
    static void ReadWait(void* data);

};


#endif
