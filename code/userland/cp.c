/// Creates files specified on the command line.

#include "syscall.h"

#define ARGC_ERROR    "Error: missing argument.\n"
#define OPEN_ERROR    "Error: not open file\n"
#define CREATE_ERROR    "Error: dont create file\n"


static void CopiarArchivo(OpenFileId fid1, OpenFileId fid2) {

    int byte = 1;
    char caracter[1] = {'\0'};
    while(byte != 0){
        caracter[0] = '\0';
        byte = Read(caracter, 1, fid1);
        Write(caracter, 1, fid2);
    }
    Close(fid1);
    Close(fid2);
    return;
}


int
main(int argc, char *argv[])
{
    if (argc < 2) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    OpenFileId archivoCopia;
    int archivoACopiar =  Create(argv[1], 0);
    if(archivoACopiar) {
        Write(CREATE_ERROR, sizeof(CREATE_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }
    else
        archivoCopia = Open(argv[1]);

    const OpenFileId archivoOriginal = Open(argv[0]);

    if(archivoOriginal > 0 && archivoCopia > 0)
        CopiarArchivo(archivoOriginal, archivoCopia);
    else{
        Write(OPEN_ERROR, sizeof(OPEN_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    return 0;
}


