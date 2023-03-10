#include "debug.h"
#include "prog.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>

#if DEBUG > 1
static FILE *debug_stream = NULL;
#endif

void debug_init(void)
{
#if DEBUG > 1
	char filename[512];
	const char *dir = getenv("CMUS_HOME");

	if (!dir || !dir[0]) {
		dir = getenv("HOME");
		if (!dir)
			die("error: environment variable HOME not set\n");
	}
	snprintf(filename, sizeof(filename), "%s/cmus-debug.txt", dir);

	debug_stream = fopen(filename, "w");
	if (debug_stream == NULL)
		die_errno("error opening `%s' for writing", filename);
#endif
}

/* This function must be defined even if debugging is disabled in the program
 * because debugging might still be enabled in some plugin.
 */
void _debug_bug(const char *function, const char *fmt, ...)
{
	const char *format = "\n%s: BUG: ";
	va_list ap;

	/* debug_stream exists only if debugging is enabled */
#if DEBUG > 1
	fprintf(debug_stream, format, function);
	va_start(ap, fmt);
	vfprintf(debug_stream, fmt, ap);
	va_end(ap);
#endif

	/* always print bug message to stderr */
	fprintf(stderr, format, function);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(127);
}

void _debug_print(const char *function, const char *fmt, ...)
{
#if DEBUG > 1
	va_list ap;

	fprintf(debug_stream, "%s: ", function);
	va_start(ap, fmt);
	vfprintf(debug_stream, fmt, ap);
	va_end(ap);
	fflush(debug_stream);
#endif
}

uint64_t timer_get(void)
{
#if DEBUG > 1
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1e6L + tv.tv_usec;
#else
	return 0;
#endif
}

void timer_print(const char *what, uint64_t usec)
{
#if DEBUG > 1
	uint64_t a = usec / 1e6;
	uint64_t b = usec - a * 1e6;

	_debug_print("TIMER", "%s: %11u.%06u\n", what, (unsigned int)a, (unsigned int)b);
#endif
}
