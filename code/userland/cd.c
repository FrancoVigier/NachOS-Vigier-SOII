#include "syscall.h"
#include "lib.c"

#define ARGC_ERROR "Error: missing argument.\n"
#define DIR_ERROR "Error: bad reference to dir.\n"

int
main(int argc, char *argv[])
{
    if (argc < 1) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }
    if(Cd(argv[0]))
        Write(DIR_ERROR, strlen(DIR_ERROR), CONSOLE_OUTPUT);
    return 0;
}
