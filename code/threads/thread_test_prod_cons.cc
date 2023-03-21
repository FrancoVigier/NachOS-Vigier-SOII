/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_prod_cons.hh"

#include <stdio.h>
#include "condition.hh"
#include "lock.hh"

static int itemCount;
static Lock estanteria("Acceso a la Estanteria");
static Condition Lleno("Estanteria Llena", &estanteria);
static Condition Vacio("Estanteria Vacia", &estanteria);
static int BUFFER_SIZE = 1000;
static int ITERACIONES = 10000;

static void productor(void* _){
	for (int i = 0; i < ITERACIONES; ++i) {
	    estanteria.Acquire();
	    while(itemCount == BUFFER_SIZE){
		    printf("Productor: Estanteria llena, nos dormimos\n");
		    Lleno.Wait();
		    printf("Productor: Estanteria con espacio, nos levantamos\n");
	    }
	    itemCount++;
	    printf("Productor: Producimos 1 item, total: %i\n",itemCount);
	    if(itemCount >= 1) {
			Vacio.Signal();
		}
	    estanteria.Release();
    }
}

static void consumidor(void* _){
	for (int i = 0; i < ITERACIONES; ++i) {
		estanteria.Acquire();
		while (itemCount == 0) {
			printf("Consumidor: No podemos consumir, nos dormimos\n");
			Vacio.Wait();
			printf("Consumidor: Podemos consumir, nos levantamos\n");
		}
		itemCount--;
		printf("Consumidor: Consumimos 1 item, total: %i\n", itemCount);
		if (itemCount == BUFFER_SIZE - 1) {
			Lleno.Signal();
		}
		estanteria.Release();
	}
}

void
ThreadTestProdCons()
{
    Thread* Productor = new Thread("Productor",true);
    Thread* Consumidor = new Thread("Consumidor",true);

    Productor->Fork(productor, nullptr);
    Consumidor->Fork(consumidor, nullptr);

    Productor->Join();
    Consumidor->Join();
}
