#include "syscall.h"
#include "lib.c"

int
main()
{
	puts("strlen(\"hola\") == ");

	char str[50];
	itoa(strlen("hola"), str);
	puts(str);

	puts("\n");

	itoa(100000, str);
	puts(str);
	puts("\n");
}