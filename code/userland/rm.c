#include "syscall.h"

#define ARGC_ERROR    "Error: missing argument.\n"
#define REMOVE_ERROR  "Error: could not remove some files.\n"

int
main(int argc, char *argv[])
{
    if (argc < 1) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }
    int ok = 1;
    for (unsigned i = 0; i < argc; i++) { ///Remueve hasta el primer archivo que no se puede remover
        if (Remove(argv[i], 0))
            ok = 0;
        if (!ok){
            Write(REMOVE_ERROR, sizeof(REMOVE_ERROR) - 1, CONSOLE_OUTPUT);
            break;
        }
    }
    return !ok;
}
