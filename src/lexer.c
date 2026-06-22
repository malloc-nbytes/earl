#include "lexer.h"
#include "map.h"
#include "error.h"

#include <assert.h>
#include <string.h>

MAP_DEFINE(const char *, token_kind, opmap);
MAP_IMPL  (const char *, token_kind, opmap);

static opmap g_opmap = {0};

static int
opmap_cmp(const char **s0,
          const char **s1)
{
        return strcmp(*s0, *s1);
}

static unsigned
opmap_hash(const char **s)
{
        // TODO: make better
        return (unsigned)**s;
}

void
lexer_init_translation_unit(void)
{
        g_opmap = opmap_create(opmap_hash, opmap_cmp);
}

static token *
token_from(const char *st,
           size_t      len,
           token_kind  k,
           size_t      r,
           size_t      c,
           const char *path)
{
        token *t;

        if (!(t = (token *)malloc(sizeof(*t))))
                fatal("could not alloc token");

        t->lx = sv_from(st, (ssize_t)len);
        t->kind = k;
        t->loc = location_from(r, c, sv_from(path, -1));

        return t;
}

lexer
lexer_from(const char *path, char *src)
{
        lexer l;
        size_t i, r, c, n;

        l = (lexer) {
                .tokens = array_empty(tokenp_ar),
                .hd = NULL,
                .err = {
                        .loc = {0},
                        .msg = NULL,
                },
        };

        i = 0, r = 1,
        c = 1, n = strlen(src);

        while (i < n) {
                char ch = src[i];

                if (ch == ' ' || ch == '\t') {
                        i += 1, c += 1;
                } else if (ch == '\n') {
                        i += 1, c = 1, r += 1;
                } else if (ch == '_' || isalpha(ch)) {
                        assert(0);
                } else if (isdigit(ch)) {
                        assert(0);
                }
        }

        return l;
}