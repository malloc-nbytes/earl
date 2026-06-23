#include "kw.h"
#include "compatibility.h"

#include <string.h>

int
kw_iskw(sv view)
{
	static const char *kwds[] = KW_CPL;
	const char *s;

	s = sv_cstr(view);

	for (size_t i = 0; i < sizeof(kwds)/sizeof(*kwds); ++i) {
		if (!strcmp(kwds[i], s))
			return 1;
	}

	return 0;
}
