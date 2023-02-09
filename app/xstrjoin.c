#include "xstrjoin.h"
#include "xmalloc.h"
#include "utils.h"

char *xstrjoin_slice(struct slice slice)
{
	const char **str = slice.ptr;
	size_t i, pos = 0, len = 0;
	char *joined;
	size_t *lens;

	lens = xnew(size_t, slice.len);
	for (i = 0; i < slice.len; i++) {
		lens[i] = strlen(str[i]);
		len += lens[i];
	}

	joined = xnew(char, len + 1);
	for (i = 0; i < slice.len; i++) {
		memcpy(joined + pos, str[i], lens[i]);
		pos += lens[i];
	}
	joined[len] = 0;

	free(lens);

	return joined;
}
