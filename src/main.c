#include "lexer.h"
#include "io.h"
#include "compatibility.h"

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
        ++argv, --argc;

        lexer_init_translation_unit();

        lexer l = lexer_from(*argv, load_file(*argv));
        lexer_dump(l);

        return 0;
}
