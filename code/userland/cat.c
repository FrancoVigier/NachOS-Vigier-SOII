#include "syscall.h"

#define ARGC_ERROR    "Error: missing argument.\n"
#define OPEN_ERROR1   "Error: cant open first file.\n"
#define OPEN_ERROR2   "Error: cant open second file.\n"

static void MostrarArch(OpenFileId fid) { ///IGUAL A CP

    int byte = 1;
    char character[1] = {'\0'};
    while(byte != 0){ // For reading without big static memory
        character[0] = '\0';
        byte = Read(character, 1, fid);
        Write(character, 1, CONSOLE_OUTPUT);
    }
    Close(fid);
    return;
}

int
main(int argc, char *argv[])
{
    if (argc < 1) { ///Errror 1
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    OpenFileId primerArchivo;
    OpenFileId segundoArchivo;

    if(argc == 2) {
        primerArchivo = Open(argv[0]);

        if(primerArchivo > 0)
            MostrarArch(primerArchivo);
        else{
            Write(OPEN_ERROR1, sizeof(OPEN_ERROR1) - 1, CONSOLE_OUTPUT);
            Exit(1);
        }

        segundoArchivo = Open(argv[1]);
        if(segundoArchivo > 0)
            MostrarArch(segundoArchivo);
        else{
            Write(OPEN_ERROR2, sizeof(OPEN_ERROR2) - 1, CONSOLE_OUTPUT);
            Exit(1);
        }
        return 0;
    }

    primerArchivo = Open(argv[0]);

    if(primerArchivo > 0)
        MostrarArch(primerArchivo);
    else{
        Write(OPEN_ERROR1, sizeof(OPEN_ERROR1) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }
    return 0;
}


