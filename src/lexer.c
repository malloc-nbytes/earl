#include "lexer.h"
#include "map.h"
#include "kw.h"
#include "error.h"
#include "compatibility.h"

#include <assert.h>
#include <string.h>
#include <ctype.h>

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

static const char *
token_kind_cstr(token_kind k)
{
        switch (k) {
        case TOKEN_KIND_EOF:        return "EOF";
        case TOKEN_KIND_INTLIT:     return "INTLIT";
        case TOKEN_KIND_STRLIT:     return "STRLIT";
        case TOKEN_KIND_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_KIND_KEYWORD:    return "KEYWORD";
        default:                    return "UNKNOWN";
        }
        unreachable();
}

static size_t
consume_while(const char  *st,
              int        (*predicate)(int))
{
        size_t i;
        for (i = 0; st[i] && predicate(st[i]); ++i);
        return i;
}

static inline int
notquote(int ch)
{
        return ch != (int)'"';
}

static inline int
notsinglequote(int ch)
{
        return ch != (int)'\'';
}

static inline int
isident(int ch)
{
        return ch == '_' || isalnum(ch);
}

void
lexer_dump(lexer l)
{
        for (size_t i = 0; i < l.tokens.len; ++i) {
                const token *t = l.tokens.data[i];
                printf("{ %s:%zu:%zu ", sv_cstr(t->loc.path), t->loc.r, t->loc.c);
                printf("lx=%s, k=%s }\n", sv_cstr(t->lx), token_kind_cstr(t->kind));
        }
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
                        size_t len = consume_while(src+i, isident);
                        token *t = token_from(src+i, len, TOKEN_KIND_IDENTIFIER, r, c, path);
                        if (kw_iskw(t->lx))
                                t->kind = TOKEN_KIND_KEYWORD;
                        array_append(l.tokens, t);
                        i += len, c += len;
                } else if (isdigit(ch)) {
                        size_t len = consume_while(src+i, isdigit);
                        token *t = token_from(src+i, len, TOKEN_KIND_INTLIT, r, c, path);
                        array_append(l.tokens, t);
                        i += len, c += len;
                } else if (ch == '"' || ch == '\'') {
                        size_t len = consume_while(src+i+1, ch == '"' ? notquote : notsinglequote);
                        token *t = token_from(src+i+1, len, TOKEN_KIND_STRLIT, r, c, path);
                        array_append(l.tokens, t);
                        i += len+2, c += len+2;
                } else {
                        assert(0);
                }
        }

        return l;
}
