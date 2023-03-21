/// Copyright (c) 2019-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "transfer.hh"
#include "lib/utility.hh"
#include "threads/system.hh"

//En si los Terminadores \0 no me importa mucho llevarlos a mem. Pero cuando necesito traer nbytes de la memoria o una string de la memoria se lo pongo para el usuario


int PASADAS_DE_LECTURA = 5;

void ReadBufferFromUser(int userAddress, char *outBuffer,
                        unsigned byteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outBuffer != nullptr);
    ASSERT(byteCount != 0);

    unsigned count = 0;
    do {
        int temp;
        count++;
        int i;
        for(i = 0; (i<PASADAS_DE_LECTURA) && (!(machine->ReadMem(userAddress, 1, &temp))); i++);
        userAddress++;
        ASSERT(i<PASADAS_DE_LECTURA);
        *outBuffer++ = (unsigned char) temp;
    } while (count < byteCount);



}


bool ReadStringFromUser(int userAddress, char *outString,
                        unsigned maxByteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outString != nullptr);
    ASSERT(maxByteCount != 0);

    unsigned count = 0;
    do {
        int temp;
        count++;
        int i;
        for(i = 0; (i<PASADAS_DE_LECTURA) && (!(machine->ReadMem(userAddress, 1, &temp))); i++);
        userAddress++;
        ASSERT(i<PASADAS_DE_LECTURA);
        *outString = (unsigned char) temp;
    } while (*outString++ != '\0' && count < maxByteCount);

    return *(outString - 1) == '\0';
}

void WriteBufferToUser(const char *buffer, int userAddress,
                       unsigned byteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(buffer != nullptr);
    ASSERT(byteCount != 0);

    unsigned count = 0;
    do {
        int i;
        for(i = 0; (i<PASADAS_DE_LECTURA) && (!(machine->WriteMem(userAddress, 1, buffer[count]))); i++);
        userAddress++;
        ASSERT(i<PASADAS_DE_LECTURA);
        count++;
        // *outBuffer = (unsigned char) temp;
    } while (count < byteCount);
}

void WriteStringToUser(const char *string, int userAddress)
{

    ASSERT(userAddress != 0);
    ASSERT(string != nullptr);

    unsigned count = 0;
    do {
        int i;
        for(i = 0; (i<PASADAS_DE_LECTURA) && (!(machine->WriteMem(userAddress, 1, string[count]))); i++);
        userAddress++;
        ASSERT(i<PASADAS_DE_LECTURA);
        count++;
    } while ((string[count]) != '\0' );

}
