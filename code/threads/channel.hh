/// Channel variables, a communication primitive
///
/// A data structure for IPC.
///
/// All synchronization objects have a `name` parameter in the constructor;
/// its only aim is to ease debugging the program.
///
/// Copyright (c)  2022 Franco Vallejos Vigier.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_THREADS_CHANNEL__HH
#define NACHOS_THREADS_CHANNEL__HH

#include "condition.hh"
#include "lock.hh"
#include "lib/list.hh"


/// This class defines a “interprocess channel”.
///
/// These are the two operations on Channel:
///
/// * `Send` -- send a message(int).
/// * `Receive` -- receive a message(int*)
///

class Lock;
class Condition;

class Channel {
public:

    Channel(const char *debugName);

    ~Channel();

    const char *GetName() const;

    /// The two operations on interprocess Channels.
    void Send(int mensaje);
    void Receive(int* mensaje);
private:
    const char *name;
    Lock* Acceso;
    Condition* BloqueanteSend;
    Condition* BloqueanteReceive;
    List<int> *buzon;
};


#endif
