#ifndef CMUS_DEBUG_H
#define CMUS_DEBUG_H

#include "compiler.h"
#ifdef HAVE_CONFIG
#include "config/debug.h"
#endif

#include <errno.h>
#include <stdint.h>

void debug_init(void);
void _debug_bug(const char *function, const char *fmt, ...) CMUS_FORMAT(2, 3) CMUS_NORETURN;
void _debug_print(const char *function, const char *fmt, ...) CMUS_FORMAT(2, 3);

uint64_t timer_get(void);
void timer_print(const char *what, uint64_t usec);

#define BUG(...) _debug_bug(__FUNCTION__, __VA_ARGS__)

#define CMUS_STR(a) #a

#define BUG_ON(a)			\
do {					\
	if (unlikely(a))		\
		BUG("%s\n", CMUS_STR(a));	\
} while (0)

#define d_print(...) _debug_print(__FUNCTION__, __VA_ARGS__)

#endif
