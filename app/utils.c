//
// Created by dingjing on 2/2/23.
//

#include "utils.h"

#include <string.h>

const char* path_get_extension (const char* filename)
{
    const char* ext;

    ext = filename + strlen (filename) - 1;
    while (ext >= filename && *ext != '/') {
        if ('.' == *ext) {
            ++ext;
            return ext;
        }
        --ext;
    }

    return NULL;
}

static inline uint32_t hash_str (const char* s)
{
    uint32_t h = 5381;
    const unsigned char* p = (const unsigned  char*) s;

    while (*p) {
        h *= 33;
        h ^= *p++;
    }

    return h ^ (h >> 16);
}
