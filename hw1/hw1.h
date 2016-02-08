#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define USAGE(name) fprintf(stderr,	"\n%s\n Usage:\t./mstat [OPTION]\n\t./mstat -h\tDisplays this help menu\n\t./mstat -i [-u]\tDisplays statistics about instruction types.\n\t./mstat -r [-u]\tDisplays information about the registers.\n\t./mstat -o [-u]\tDisplays number and percentage of opcodes used.\n\n\tOptional flags:\n\t\t-u\tDisplays human readable headers for the different outputs.\n", name);

void rflagHandler(int uflag);

void iflagHandler(int uflag);

void oflagHandler(int uflag);