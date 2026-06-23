#include "compatibility.h"

#if HAVE_STRLEN == 0
size_t
strlen(const char *s)
{
	size_t i;
	for (i = 0; s[i]; ++i);
	return i;
}
#endif
