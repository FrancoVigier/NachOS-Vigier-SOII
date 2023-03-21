/// Routines for threads' channel.
///
/// Copyright (c)  2022 Franco Vallejos Vigier.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "channel.hh"
#include "condition.hh"
#include "lock.hh"
#include "system.hh"
#include <cstdio>
Channel::Channel(const char *debugName)
{
    name = debugName;
    Acceso = new Lock("Access Channel Lock");
    BloqueanteSend = new Condition(debugName, Acceso);
    BloqueanteReceive = new Condition(debugName, Acceso);
    buzon = new List <int>;
}

Channel::~Channel()
{
    delete BloqueanteSend;
    delete BloqueanteReceive;
    delete Acceso;
    delete buzon;
}

const char *
Channel::GetName() const
{
    return name;
}

void
Channel::Send(int mensaje)
{
    Acceso->Acquire(); //Solo 1 puede operar a la vez en el canal
    buzon->Append(mensaje);


    BloqueanteReceive->Signal();
    int b = 0;
     if(buzon->Head()) {
       b = buzon->Head();
    }

    while(! buzon->IsEmpty() && b == buzon->Head())
        BloqueanteSend->Wait();

  //  if(BloqueanteReceive->SleepingThreadAmount() != 0){//Si hay un proceso que puso un Receive y esta esperando dormido le mando un Signal y me duermo a la espera que este me levante para completar la operacion
  //      BloqueanteReceive->Signal();
  //      BloqueanteSend->Wait();
 //   } else {
//	    BloqueanteSend->Wait();//Si no hay procesos esperando un Send, entonces me tengo que ir a dormir y esperar mi Receive
//    }
    Acceso->Release(); //Si llego aqui es porque me levante de la siesta o habia un nodo que esperaba mi Send, de ambas formas suelto el Lock de Acceso
}

void
Channel::Receive(int* mensaje)
{
    Acceso->Acquire(); //Solo 1 puede operar a la vez en el canal

    while(buzon->IsEmpty())
        BloqueanteReceive->Wait();

 //   if(BloqueanteSend->SleepingThreadAmount() != 0) {//Si hay un proceso esperando mi receive le mando mi signal
//	    BloqueanteSend->Signal();
//    } else {//Si no hay un Proceso esperando mi Receive entonces me voy a dormir hasta que uno haga signal y suelto el lock del receptor
//	    BloqueanteReceive->Wait();
//    }
    *mensaje = buzon->Pop(); //Aca si puedo capturar el mensaje ya que me garantizo que hay por lo menos uno en el buzon
    BloqueanteSend->Signal();
    Acceso->Release();
}

