/// Copyright (c) 2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "synch_console.hh"
#include <stdlib.h>


SynchConsole::SynchConsole(const char* in,const char* out){
    rLock = new Lock("Read Lock");
    rSem = new Semaphore("Read Sem",0);

    wLock = new Lock("Write Lock");
    wSem = new Semaphore("Write Sem", 0);
    console = new Console(in,out,ReadWait,WriteOK,this);
}

SynchConsole::~SynchConsole(){
    delete rLock;
    delete rSem;
    delete wLock;
    delete wSem;
    delete console;
}

void SynchConsole:: WriteCharToConsol(char c){
    wLock ->Acquire();
    console->PutChar(c);
    wSem->P();
    wLock->Release();
}

char SynchConsole::ReadCharFromConsole(){
    rLock->Acquire();
    rSem->P();
    char c = console->GetChar();
    rLock->Release();
    return c;
}

void SynchConsole::WriteOK(void* data){
    ((SynchConsole*)data)->wSem->V();
}

void SynchConsole::ReadWait(void* data){
    ((SynchConsole*)data)->rSem->V();
}
