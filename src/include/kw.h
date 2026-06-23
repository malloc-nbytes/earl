#ifndef KW_H_INCLUDED
#define KW_H_INCLUDED

#include "sv.h"

#define KW_LET "let"
#define KW_IF "if"
#define KW_ELSE "else"

#define KW_CPL { \
	KW_LET, \
	KW_IF, \
	KW_ELSE, \
}

int kw_iskw(sv view);

#endif // KW_H_INCLUDED
