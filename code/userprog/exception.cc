/// Entry points into the Nachos kernel from user programs.
///
/// There are two kinds of things that can cause control to transfer back to
/// here from user code:
///
/// * System calls: the user code explicitly requests to call a procedure in
///   the Nachos kernel.  Right now, the only function we support is `Halt`.
///
/// * Exceptions: the user code does something that the CPU cannot handle.
///   For instance, accessing memory that does not exist, arithmetic errors,
///   etc.
///
/// Interrupts (which can also cause control to transfer from user code into
/// the Nachos kernel) are handled elsewhere.
///
/// For now, this only handles the `Halt` system call.  Everything else core-
/// dumps.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "transfer.hh"
#include "syscall.h"
#include "filesys/directory_entry.hh"
#include "threads/system.hh"
#include "args.hh"

#include <stdio.h>
#include "address_space.hh"

#define MAX_LENGHT_ARG 200
static void
IncrementPC()
{
    unsigned pc;

    pc = machine->ReadRegister(PC_REG);
    machine->WriteRegister(PREV_PC_REG, pc);
    pc = machine->ReadRegister(NEXT_PC_REG);
    machine->WriteRegister(PC_REG, pc);
    pc += 4;
    machine->WriteRegister(NEXT_PC_REG, pc);
}

///Rutina Para Guardar los argumentos de un hilo que hace Exec()
void
RutinaHiloSCExec(void* argv)
{
	currentThread->space->InitRegisters();
	currentThread->space->RestoreState();

	int argc;
	if (argv != nullptr) {
		argc = WriteArgs((char **) argv); //Leo la cantidad de arg
	} else {
		argc = 0;
		machine->Run();
		return;
	}
	machine->WriteRegister(4, argc); //Escribo en r4 la cantidad de argumentos como indica el enunciado
	int stackpoint = machine->ReadRegister(STACK_REG);
	machine->WriteRegister(5,stackpoint);
	machine->WriteRegister(STACK_REG,stackpoint-24); //La direccion de donde comenzar a leer esos n args, con 24 de convencion de llamada de mips
	machine->Run();
}

/// Do some default behavior for an unexpected exception.
///
/// NOTE: this function is meant specifically for unexpected exceptions.  If
/// you implement a new behavior for some exception, do not extend this
/// function: assign a new handler instead.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
static void
DefaultHandler(ExceptionType et)
{
    int exceptionArg = machine->ReadRegister(2);

    fprintf(stderr, "Unexpected user mode exception: %s, arg %d.\n",
            ExceptionTypeToString(et), exceptionArg);
	ASSERT(false);
}

/// Handle a system call exception.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
///
/// The calling convention is the following:
///
/// * system call identifier in `r2`;
/// * 1st argument in `r4`;
/// * 2nd argument in `r5`;
/// * 3rd argument in `r6`;
/// * 4th argument in `r7`;
/// * the result of the system call, if any, must be put back into `r2`.
///
/// And do not forget to increment the program counter before returning. (Or
/// else you will loop making the same system call forever!)
static void
SyscallHandler(ExceptionType _et)
{
    int scid = machine->ReadRegister(2);

    switch (scid) {

        case SC_HALT:
            DEBUG('e', "Shutdown, initiated by user program.\n");
		    machine->WriteRegister(2,0);
            interrupt->Halt();
            break;

        case SC_CREATE: {
            int filenameAddr = machine->ReadRegister(4);
            bool isDirectory = machine->ReadRegister(5);

            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, 1);
                break;
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, 1);
                break;
            }

            DEBUG('e', "`Create` requested for file `%s`.\n", filename);


            #ifdef FILESYS
            if(isDirectory && !fileSystem->CreateDir(filename)){
                DEBUG('e', "Error: Directory not created.\n");
                machine->WriteRegister(2, 1);
                break;
            }
            #endif


            if(! isDirectory && !fileSystem->Create(filename, 0)){
                DEBUG('e',"NO Success Creation\n");
                machine->WriteRegister(2, 1);
                break;
            }else{
                DEBUG('e',"Success Creation\n");
                machine->WriteRegister(2,0);
            }
            break;
        }

        case SC_EXIT: { ///Ej 2a. P3
            int r = machine->ReadRegister(4); //Leo R4 usrAddress
			DEBUG('e',"Return: %d\n", r);

            currentThread->Finish(r); //Tengo que devolverle al thread si termino bien o no
            break;
        }

        case SC_REMOVE: { ///Ej 2a. P3
            int fileDir = machine-> ReadRegister(4);
            bool isDirectory = machine->ReadRegister(5);
            ///ERROR 1
            if(fileDir == 0){
                machine->WriteRegister(2,1); //Returno por r2 el errror de la syscall
                DEBUG('e',"Archivo No Exist\n");
                break;
            }
            ///
            char fileName[FILE_NAME_MAX_LEN +1];
            ///ERROR 2
            if(!ReadStringFromUser(fileDir,fileName,FILE_NAME_MAX_LEN +1)){
                machine->WriteRegister(2,1); //Returno por r2 el errror de la syscall
                DEBUG('e',"File Name extra Long\n");
                break;
            }
            ///

            #ifdef FILESYS
            if(isDirectory) {
                DEBUG('e', "Cannot delete directory.\n");
                machine->WriteRegister(2, -2);
				break;
            }
            #endif


            ///ERROR 3
            if(!isDirectory && !fileSystem->Remove(fileName)){
                machine->WriteRegister(2,1); //Returno por r2 el errror de la syscall
                DEBUG('e',"Cant Remove File\n");
                break;
            }
            ///
            DEBUG('e',"File Removed\n");
            machine->WriteRegister(2,0); //Retorno OK por r2
            break;
        }

        case SC_WRITE: { ///Ej 2b,d .P3
            int bufferDir = machine ->ReadRegister(4);
            ///ERROR 1
            if(bufferDir == 0){
                machine->WriteRegister(2,0); //Returno por r2, la cantidad de bytes escritos
                DEBUG('e',"Buffer No Exist\n");
                break;
            }
            ///
            int sizeBytes = machine-> ReadRegister(5);
            ///ERROR 2
            if(sizeBytes < 0){
                machine->WriteRegister(2,0); //Returno por r2, la cantidad de bytes escritos
                DEBUG('e',"Size of Buffer is Zero\n");
                break;
            }
            ///
            OpenFileId fdId = machine->ReadRegister(6);

            if(fdId < 0){
                DEBUG('e', "Invalid file descriptor ID to Read\n");    //to avoid the assert in Remove function
                machine->WriteRegister(2, 0);
                break;
            }

            ///ERROR 4, parte del 2d
            if(fdId == CONSOLE_INPUT){
                machine->WriteRegister(2,0);
                DEBUG('e', "Write on Console Input\n");
                break;
            }

            char* buffer = new char[sizeBytes + 1];
            ReadBufferFromUser(bufferDir, buffer, sizeBytes);

            if(fdId == CONSOLE_OUTPUT) {
                for(int i = 0; i < sizeBytes; i++)
                    synchConsole->WriteCharToConsol(buffer[i]);

                machine->WriteRegister(2, sizeBytes);
            } else if(!currentThread->opFD->HasKey(fdId)) {
                DEBUG('e', "File doesnt Exist\n");
                machine->WriteRegister(2, 0);
            } else {

                OpenFile* file = currentThread->opFD->Get(fdId);

                int bytesWrited = file->Write(buffer, sizeBytes);

                if(bytesWrited <= 0) {
                    machine->WriteRegister(2, 0);
                } else {
                    machine->WriteRegister(2, bytesWrited);
                }
            }

            delete [] buffer;


            ///
            ///ERROR 5
          //  if(!currentThread->opFD->HasKey(fdId)){
          //      machine->WriteRegister(2,0);
          //      DEBUG('e', "File doesnt Exist\n");
          //      break;
          //  }
            ///

          //  char buffer [sizeBytes +1];
          //  ReadBufferFromUser(bufferDir,buffer,sizeBytes); //Tengo en buffer ya construida la cadena a escribir en el file
          //  buffer[sizeBytes] = 0;

	  //      DEBUG('e', "Writing \"%s\"\n", buffer);

	    //    int bytesWrite = 0;

        //    if(fdId != CONSOLE_OUTPUT){ //Escritura normal del file
        //        OpenFile* file = currentThread->opFD->Get(fdId);
        //        bytesWrite = file->Write(buffer,sizeBytes);
        //    }
         //   else{ //Vamos a STDOUT Ej 2d
          //      for(int i = 0; i<sizeBytes; i++)
          //          synchConsole->WriteCharToConsol(buffer[i]);
          //      bytesWrite = sizeBytes;
//}
           // DEBUG('e', "File edited Success\n");
//machine->WriteRegister(2, bytesWrite);
            break;
        }

        case SC_READ: { ///Ej 2b,d .P3
            int bufferDir = machine ->ReadRegister(4);
            ///ERROR 1
            if(bufferDir == 0){
                machine->WriteRegister(2,0); //Returno por r2, la cantidad de bytes escritos
                DEBUG('e',"Buffer No Exist\n");
                break;
            }
            ///
            int sizeBytes = machine-> ReadRegister(5);
            ///ERROR 2
            if(sizeBytes <= 0){
                machine->WriteRegister(2,0); //Returno por r2, la cantidad de bytes escritos
                DEBUG('e',"Size of Buffer is Zero\n");
                break;
            }
            ///
            OpenFileId fdId = machine->ReadRegister(6);

            if(fdId < 0){
                DEBUG('e', "Invalid file descriptor ID to Read\n");    //to avoid the assert in Remove function
                machine->WriteRegister(2, 0);
                break;
            }

            char* buffer = new char[sizeBytes + 1];

            if(fdId == CONSOLE_INPUT) {
                DEBUG('e', "Reading console...\n");

                for(int i = 0; i < sizeBytes; i++)
                    buffer[i] = synchConsole->ReadCharFromConsole();

                WriteBufferToUser(buffer, bufferDir, sizeBytes);
                machine->WriteRegister(2, sizeBytes);
            } else if(!currentThread->opFD->HasKey(fdId)) {
                DEBUG('e', "Read in unopen file\n");
                machine->WriteRegister(2, 0);
            } else {
                OpenFile* file = currentThread->opFD->Get(fdId);

                int bytesRead = file->Read(buffer, sizeBytes);
                if(bytesRead <= 0) {
                    machine->WriteRegister(2, 0);
                } else {
                    WriteBufferToUser(buffer, bufferDir, sizeBytes);
                    machine->WriteRegister(2, bytesRead);
                }
            }
            delete [] buffer;
/*
            ///ERROR 4, parte del 2d
            if(fdId == CONSOLE_OUTPUT){
                machine->WriteRegister(2,0);
                DEBUG('e', "Read on Console Output\n");
                break;
            }
            ///
            ///ERROR 5
            if(!currentThread->opFD->HasKey(fdId)){
                machine->WriteRegister(2,0);
                DEBUG('e', "File doesnt Exist\n");
                break;
            }
            ///
            char buffer [sizeBytes];

            int bytesRead = 0;

            if(fdId != CONSOLE_INPUT){ //Lectura normal del file
                OpenFile* file = currentThread->opFD->Get(fdId);
                bytesRead = file->Read(buffer,sizeBytes);
            } else { //Lectura de la STDIN. Ej 2d. P3
                int i=0;
                for(i = 0; i < sizeBytes; i++){
                    buffer[i] = synchConsole->ReadCharFromConsole();
                }
                buffer[i] =0;
                bytesRead = i;
            }
            if(bytesRead != 0)
                WriteBufferToUser(buffer,bufferDir,bytesRead);

            DEBUG('e', "Read %u bytes\n", bytesRead);
            machine->WriteRegister(2,bytesRead);
*/
            break;
        }


        case SC_CLOSE: { ///Ej 2c. P3
            int fid = machine->ReadRegister(4);
            ///ERROR 1
            if(fid == 0 || fid == 1 || !currentThread->opFD->HasKey(fid)){
                machine->WriteRegister(2,-1); //Returno por r2 el errror de la syscall
                DEBUG('e',"Cant Close\n");
                break;
            }
            ///ERROR 2
    //        if(!currentThread->opFD->HasKey(fid)){
      //          machine->WriteRegister(2,-1); //Returno por r2 el errror de la syscall
     //           DEBUG('e',"File to close dont Exist\n");
     //           break;
     //       }
            ///
            DEBUG('e', "FD: Closed\n");

            #ifdef FILESYS
            currentThread->opFD->Get(fid)->Close();
            #endif

            currentThread->opFD->Remove(fid);
            machine->WriteRegister(2,0);
            break;
        }

        case SC_OPEN: { ///Ej 2c. P3
            int fileDir = machine-> ReadRegister(4);
            ///ERROR 1
            if(fileDir == 0){
                machine->WriteRegister(2,-1); //Returno por r2 el errror de la syscall
                DEBUG('e',"Archivo No Exist\n");
                break;
            }
            ///
            char fileName[FILE_NAME_MAX_LEN +1];
            ///ERROR 2
            if(!ReadStringFromUser(fileDir,fileName,FILE_NAME_MAX_LEN +1)){
                machine->WriteRegister(2,-1); //Returno por r2 el errror de la syscall
	            DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
	                  FILE_NAME_MAX_LEN);
                break;
            }
            ///
            OpenFile* file = fileSystem ->Open(fileName);
            ///ERROR 3
            if(file == nullptr){
                machine->WriteRegister(2,-1); //Returno por r2 el errror de la syscall
                DEBUG('e',"File doesnt Open\n");
                break;
            }
            ///
            int fileFD = currentThread->opFD->Add(file);
            ///ERROR 4
            if(fileFD < 0){
                machine->WriteRegister(2,-1); //Returno por r2 el errror de la syscall
                DEBUG('e',"File Descriptor Table Full\n");
                delete file;
                break;
            }
            ///
            DEBUG('e', "File Opened\n");
            machine->WriteRegister(2, fileFD); //Retorno el FD al register r2
            break;
        }

        case SC_EXEC: { ///Ej 2e
            int fileDir = machine-> ReadRegister(4);
            int argumentsPosition = machine->ReadRegister(5);
            bool joinable = machine->ReadRegister(6);

            ///ERROR 1
            if(fileDir == 0){
                machine->WriteRegister(2,-1); //Returno por r2 el errror de la syscall
                DEBUG('e',"Archivo No Exist\n");
                break;
            }
            ///
            char fileName[FILE_NAME_MAX_LEN +1];
            ///ERROR 2
            if(!ReadStringFromUser(fileDir,fileName,FILE_NAME_MAX_LEN +1)){
                machine->WriteRegister(2,-1); //Returno por r2 el errror de la syscall
                DEBUG('e',"File Name extra Long\n");
                break;
            }
            ///

            char **argv = nullptr;
            if (argumentsPosition) {
                argv = SaveArgs(argumentsPosition);
            }

            OpenFile* file = fileSystem ->Open(fileName);

	        if (file == nullptr) {
		        DEBUG('e', "File doesnt Exist\n");
		        machine->WriteRegister(2, -1);
		        break;
	        }

	        DEBUG('e', "Executing \"%s\"\n", fileName);


	        DEBUG('e', "Args: Address \"0x%x\", Joinable %s\n", argumentsPosition, joinable?"yes":"no");

	        Thread *hilo = new Thread(fileName, joinable, 0);

            AddressSpace *space = new AddressSpace(file, hilo->tId);

            hilo->space = space;

/*
	        #ifdef SWAP
                AddressSpace *mem = new AddressSpace(file, hilo->tId);
            #else
                AddressSpace *mem = new AddressSpace(file, 0);
            #endif

	  //      AddressSpace *mem = new AddressSpace(file);
	        hilo->space = mem;
*/

	//		if(argumentsPosition)
	//			argv = SaveArgs(argumentsPosition);

	       // delete file;
            #ifndef DEMAND_LOADING
                delete file;
            #endif

            hilo->Fork(RutinaHiloSCExec, (void *) argv);
	        machine->WriteRegister(2, hilo->tId);
	        break;
        }
        case SC_JOIN: {
            int tidJoin = machine->ReadRegister(4);
            ///ERROR 1
            if(!threadUPS->HasKey(tidJoin)){
                machine->WriteRegister(2,-1);
                DEBUG('e',"Thread doesnt Exist\n");
                break;
            }
            ///
            int joinReturn = threadUPS->Get(tidJoin)->Join();
            machine->WriteRegister(2,joinReturn);
            DEBUG('e',"Thread Joined :)\n");
            break;
        }
        case SC_PS:{ ///Ej 3. Opcional. Asumo que la Operacion que Printea el Estado del Scheduler es SCHEDULER::PRINT
            DEBUG('e',"Scheduler State\n");
            scheduler->Print();
            break;
        }
        case SC_LS: {
            #ifdef FILESYS
            int canal = machine->ReadRegister(4);
            char* lsResult = new char[200];
            unsigned bytesRead = fileSystem->Ls(lsResult);

            if(bytesRead)
                WriteBufferToUser(lsResult, canal, 200);

            delete [] lsResult;
            #endif

            break;
        }

        case SC_CD: {
            #ifdef FILESYS
            int dirPath = machine->ReadRegister(4);

            if (dirPath == 0) {
                DEBUG('e', "Error: path is null.\n");
                machine->WriteRegister(2, 1);
                break;
            }

            char* path = new char[FILE_PATH_MAX_LEN + 1];

            if (!ReadStringFromUser(dirPath, path, FILE_PATH_MAX_LEN + 1)) {
                DEBUG('e', "Error: filename long limit.\n");
                delete [] path;
                machine->WriteRegister(2, 1);
                break;
            }

            bool cdOk = fileSystem->ChangeDir(path);

            if(!cdOk) {
                DEBUG('e', "Error: directory inexist.\n");
                machine->WriteRegister(2, 1);
                delete [] path;
                break;
            }
            delete [] path;
            #endif

            machine->WriteRegister(2, 0);

            break;
        }
        default:
            fprintf(stderr, "Unexpected system call: id %d.\n", scid);
            ASSERT(false);

    }

    IncrementPC();
}


static void
PageFaultExeption(ExceptionType pfE){
  #ifdef USE_TLB
    unsigned vpn = machine->ReadRegister(BAD_VADDR_REG) / PAGE_SIZE;
    TranslationEntry *pageTableentry = &(currentThread->space->pageTable[vpn]);

    #ifdef DEMAND_LOADING
    if(!pageTableentry->valid){
    DEBUG('e', "Pagina no cargada en Memoria, yendo a cargarla\n");
    currentThread->space->LoadPage(vpn);
}
    #endif

    static int index = 0;
    TranslationEntry *tlb = machine->GetMMU()->tlb;
    if(tlb[index].valid){
        unsigned victimaVirtual = tlb[index].virtualPage;
        currentThread->space->pageTable[victimaVirtual] = tlb[index];
    }
    tlb[index] = *pageTableentry;

    index++;
    index %= TLB_SIZE;
    stats->numPageFaults++;
    #else
    DefaultHandler(pfE);
    #endif // USE_TLB
}


static void
ReadOnlyException(ExceptionType _et)
{
    DEBUG('e', "Read from a page Only Read permission\n");
    ASSERT(false);
    return;
}
/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void
SetExceptionHandlers()
{
    machine->SetHandler(NO_EXCEPTION,            &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION,       &SyscallHandler);
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &PageFaultExeption);
    machine->SetHandler(READ_ONLY_EXCEPTION,     &ReadOnlyException);
    machine->SetHandler(BUS_ERROR_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION,      &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}
