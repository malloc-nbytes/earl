 #ifndef LEXER_H_INCLUDED
#define LEXER_H_INCLUDED

#include "sv.h"
#include "location.h"
#include "array.h"

typedef enum {
        TOKEN_KIND_EOF = 0,
        TOKEN_KIND_INTLIT,
        TOKEN_KIND_STRLIT,
        TOKEN_KIND_IDENTIFIER,
        TOKEN_KIND_KEYWORD,
} token_kind;

typedef struct {
        sv lx;
        token_kind kind;
        location loc;
} token;

ARRAY_DEFINE(token *, tokenp_ar);

typedef struct {
        tokenp_ar tokens;
        token *hd;
        struct {
                location loc;
                char *msg;
        } err;
} lexer;

void  lexer_init_translation_unit(void);
lexer lexer_from(const char *path, char *src);
void  lexer_dump(lexer l);

#endif // LEXER_H_INCLUDED
