#include "xmalloc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

extern char *program_name;

void malloc_fail(void)
{
	fprintf(stderr, "%s: could not allocate memory: %s\n", PACKAGE_NAME, strerror(errno));
	exit(42);
}

#ifndef HAVE_STRNDUP
char *xstrndup(const char *str, size_t n)
{
	size_t len;
	char *s;

	for (len = 0; len < n && str[len]; len++)
		;
	s = xmalloc(len + 1);
	memcpy(s, str, len);
	s[len] = 0;
	return s;
}
#endif
