#include "file_system.hh"
#include "lib/utility.hh"
#include "machine/disk.hh"
#include "machine/statistics.hh"
#include "threads/thread.hh"
#include "threads/system.hh"
#include "lib/assert.hh"

#include <stdio.h>
#include <string.h>

void
TestDirectory() {

	printf("Creo los Directorios Valle y Messu, en ./\n");
	ASSERT(fileSystem->CreateDir("valle"));
	ASSERT(fileSystem->CreateDir("messu"));
	printf("\n_________Mostrando el Dir root ./____________\n");
	fileSystem->List();
	printf("\n_____________________\n");


	ASSERT(!fileSystem->Create("/Messu/bin1"));
	ASSERT(!fileSystem->Create("/Messu/bin2"));
	ASSERT(!fileSystem->Create("/Valle/bin1"));
	ASSERT(!fileSystem->Create("/Valle/bin2"));

	printf("Creo los Archivos para el dir valle, con el path ./Dir/Arc\n");
	ASSERT(fileSystem->Create("/valle/bin2"));
    ASSERT(fileSystem->ChangeDir("/valle"));
    printf("\n_________Mostrando el Dir ./valle____________\n");
	fileSystem->List();
	printf("\n_____________________\n");
    ASSERT(fileSystem->ChangeDir("/"));


	printf("Creo el Dir de Nachos\n");
	ASSERT(fileSystem->CreateDir("nachos"));

	printf("Hago CD al dir de nachos\n");
	ASSERT(fileSystem->ChangeDir("/nachos"));

	printf("Creo las Carpetas en el ./nachos/\n");
	/// Create a lot of directories in the current directory
	ASSERT(fileSystem->CreateDir("bin"));
	ASSERT(fileSystem->CreateDir("filesys"));
	ASSERT(fileSystem->CreateDir("lib"));
	ASSERT(fileSystem->CreateDir("machine"));
	ASSERT(fileSystem->CreateDir("network"));
	ASSERT(fileSystem->CreateDir("threads"));
	ASSERT(fileSystem->CreateDir("userland"));
	ASSERT(fileSystem->CreateDir("userprog"));
	ASSERT(fileSystem->CreateDir("vmem"));
	ASSERT(fileSystem->Create("Makefile"));
	ASSERT(fileSystem->Create("aaa.txt"));

    printf("\n_________Mostrando el Dir ./nachos____________\n");
	fileSystem->List();
	printf("\n_____________________\n");



	ASSERT(fileSystem->ChangeDir("/"));
	printf("\n_________Mostrando el Dir root ./____________\n");
	fileSystem->List();
	printf("\n_____________________\n");


	printf("Removiendo direcctorio ./nachos/bin, el ./nachos/machine y el binario ./nachos/Makefile\n");
	ASSERT(fileSystem->Remove("/nachos/Makefile"));

	ASSERT(fileSystem->ChangeDir("nachos"));
    printf("\n_________Mostrando el Dir ./nachos____________\n");
	fileSystem->List();
	printf("\n_____________________\n");


	ASSERT(fileSystem->ChangeDir("/"));
	ASSERT(fileSystem->Create("/nachos/Test"));
	ASSERT(fileSystem->Remove("/nachos/Test"));

	ASSERT(fileSystem->ChangeDir("/nachos"));

	ASSERT(!fileSystem->Open("Makefile"));


	printf("\n_________Iniciando prueba de escritura de ./nachos/aaa.txt____________\n");
	{
		OpenFile *file = fileSystem->Open("aaa.txt");
		ASSERT(file);
		for (unsigned int i = 0; i < 20000; i++) {
			ASSERT(file->Write("a", 1));
		}
		file->Close();
	}
	printf("\n_________Fin prueba de escritura de ./nachos/aaa.txt____________\n");
    printf("\n_________Iniciando prueba de lectura de ./nachos/aaa.txt____________\n");
	{
		OpenFile *file = fileSystem->Open("aaa.txt");
		ASSERT(file);
		char tmp[20000];
		file->Read(tmp, 20000);
		for (unsigned int i = 0; i < 20000; ++i) {
			ASSERT(tmp[i] == 'a');
		}
		file->Close();
	}
	printf("\n_________Fin prueba de lectura de ./nachos/aaa.txt____________\n");
 }
