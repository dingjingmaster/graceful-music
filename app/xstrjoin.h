#ifndef CMUS_XSTRJOIN_H
#define CMUS_XSTRJOIN_H

#include "utils.h"

char *xstrjoin_slice(struct slice);
#define xstrjoin(...) xstrjoin_slice(TO_SLICE(const char *, __VA_ARGS__))

#endif
