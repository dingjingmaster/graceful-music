#ifndef CMUS_READ_WRAPPER_H
#define CMUS_READ_WRAPPER_H

#include "ip.h"

#include <stddef.h> /* size_t */
#include <sys/types.h> /* ssize_t */

ssize_t read_wrapper (InputPluginData* ipData, void *buffer, size_t count);

#endif
