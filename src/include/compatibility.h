#ifndef COMPATIBILITY_H_INCLUDED
#define COMPATIBILITY_H_INCLUDED

#include "config.h"

#include <stddef.h>

#if HAVE_STRLEN == 0
size_t strlen(const char *s);
#endif

#endif // COMPATIBILITY_H_INCLUDED
