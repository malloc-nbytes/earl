#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>

static void
usage(void)
{
        printf("Usage: earl <path>\n [OPTIONS...]\n");
        exit(0);
}

int
main(int argc, char *argv[])
{
        if (argc <= 1)
                usage();

        lexer_init_translation_unit();

        return 0;
}
