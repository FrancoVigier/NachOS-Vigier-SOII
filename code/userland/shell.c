#include "syscall.h"
#include "lib.c"

#define bool int
#define false 0
#define true 1

#define MAX_LINE_SIZE  60
#define MAX_ARG_COUNT  32
#define ARG_SEPARATOR  ' '

#define NULL  ((void *) 0)

static inline void
WritePrompt(OpenFileId output)
{
    static const char PROMPT[] = "--> ";
    Write(PROMPT, sizeof PROMPT - 1, output);
    return;
}

static inline void
WriteError(const char *description, OpenFileId output)
{
    // TODO: how to make sure that `description` is not `NULL`?

    static const char PREFIX[] = "Error: ";
    static const char SUFFIX[] = "\n";

    Write(PREFIX, sizeof PREFIX - 1, output);
    Write(description, strlen(description), output);
    Write(SUFFIX, sizeof SUFFIX - 1, output);
}

static unsigned
ReadLine(char *buffer, unsigned size, OpenFileId input)
{
    if(buffer == NULL)
        return 0;

    unsigned i;

    for (i = 0; i < size; i++) {
        Read(&buffer[i], 1, input);
        if (buffer[i] == '\n') {
            buffer[i] = '\0';
            break;
        }
    }

    return i;
}

static int
PrepareArguments(char *line, char **argv, unsigned argvSize)
{
    unsigned argCount = 0;

    if(argvSize > MAX_ARG_COUNT || line == NULL || argv == NULL)
        return 1;

    for (unsigned i = 0; line[i] != '\0'; i++) {
        if (line[i] == ARG_SEPARATOR) {
            if (argCount == argvSize - 1)
                return 1;
            line[i] = '\0';
            while(line[++i] == ARG_SEPARATOR);
            if(line[i] != '\0')
                argv[argCount++] = &line[i];
        }
    }

    argv[argCount] = NULL;

    return 0;
}


SpaceId ExecuteAlias(char* line, char** argv, int joinable) {

    SpaceId result = 0;

    if(argv[0] == NULL)
        argv = NULL;

    if(strcmpp(line, "echo"))
        result = Exec("echo", argv, joinable);

    else if(strcmpp(line, "exit"))
        result = Exec("halt", argv, joinable);

    else if(strcmpp(line, "cat"))
        result = Exec("cat", argv, joinable);

    else if(strcmpp(line, "write"))
        result = Exec("write", argv, joinable);

    else if(strcmpp(line, "touch"))
        result = Exec("touch", argv, joinable);

    else if(strcmpp(line, "create"))
        result = Exec("touch", argv, joinable);

    else if(strcmpp(line, "rm"))
        result = Exec("rm", argv, joinable);

    else if(strcmpp(line, "cp"))
        result = Exec("cp", argv, joinable);

    else if(strcmpp(line, "ls"))
        result = Exec("ls", argv, joinable);

    else if(strcmpp(line, "mkdir"))
        result = Exec("mkdir", argv, joinable);


    else
        result = Exec(line, argv, joinable);

    return result;
}

int
main(void)
{
    const OpenFileId      INPUT = CONSOLE_INPUT;
    const OpenFileId      OUTPUT = CONSOLE_OUTPUT;
    char                  line[MAX_LINE_SIZE];
    char                  *argv[MAX_ARG_COUNT];

    for (;;) {
        WritePrompt(OUTPUT);

        const unsigned lineSize = ReadLine(line, MAX_LINE_SIZE, INPUT);
         if (lineSize == 0) {
            continue;
        }

        const int args = PrepareArguments(line, argv, MAX_ARG_COUNT);
        if(args) {
            WriteError("Argument Issues.", OUTPUT);
            continue;
        }

        if(line[0] == '&') {

            const SpaceId newProc1 = ExecuteAlias(&line[1], argv, 0);

            if(newProc1 < 0) {
                WriteError("Bad Fork.", OUTPUT);
                continue;
            }
        } else {

            const SpaceId newProc2 = ExecuteAlias(line, argv, 1);

            if(newProc2 < 0) {
                WriteError("Bad Fork.", OUTPUT);
                continue;
            } else {
                Join(newProc2);
            }
        }
    }
    // Never reached.
    return -1;
}
