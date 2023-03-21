/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_prod_cons_priorities.hh"

#include <stdio.h>
#include "condition.hh"
#include "lock.hh"

static int itemCount;
static Lock estanteria("Acceso a la Estanteria");
static Condition noLleno("Estanteria Llena", &estanteria);
static Condition noVacio("Estanteria Vacia", &estanteria);
static int BUFFER_SIZE = 5;
static int ITERACIONES = 50;

static void producir(void* _) {
	for (int i = 0; i < ITERACIONES; ++i) {
		estanteria.Acquire();
		while (itemCount == BUFFER_SIZE) {
			printf("Productor: Estanteria llena, nos dormimos\n");
			noLleno.Wait();
			printf("Productor: Estanteria con espacio, nos levantamos\n");
		}
		itemCount++;
		printf("Productor: Producimos 1 item, total: %i\n", itemCount);
		if (itemCount > 0) {
			noVacio.Signal();
		}
		estanteria.Release();
	}
	printf("Productor: Termino!\n");
}

static void consumir(void* _) {
	for (int i = 0; i < ITERACIONES; ++i) {
		estanteria.Acquire();
		while (itemCount == 0) {
			printf("Consumidor: No podemos consumir, nos dormimos\n");
			noVacio.Wait();
			printf("Consumidor: Podemos consumir, nos levantamos\n");
		}
		itemCount--;
		printf("Consumidor: Consumimos 1 item, total: %i\n", itemCount);
		if (itemCount < BUFFER_SIZE) {
			noLleno.Signal();
		}
		estanteria.Release();
	}
}

void
ThreadTestProdConsPriorities()
{
	Thread* ProductorA = new Thread("ProductorA",true, 500);
	Thread* ProductorB = new Thread("ProductorB",true, 500);
	Thread* Consumidor = new Thread("Consumidor",true, 0);

	ProductorA->Fork(producir, nullptr);
	ProductorB->Fork(producir, nullptr);
	Consumidor->Fork(consumir, nullptr);

	ProductorA->Join();
	ProductorB->Join();
	Consumidor->Join();
}
