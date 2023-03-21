#include "syscall.h"
#include  "lib.c"
#define ARGC_ERROR "Error: in the arguments.\n"

int
main(int argc, char *argv[])
{
    if (argc > 1) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    char lsResult[200];
    Ls(lsResult);

    Write(lsResult, strlen(lsResult), CONSOLE_OUTPUT);
    return 0;
}
