//
// Created by dingjing on 2/2/23.
//

#ifndef GRACEFUL_MUSIC_UTILS_H
#define GRACEFUL_MUSIC_UTILS_H

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

const char* path_get_extension (const char* filename);

static inline uint32_t hash_str (const char* s);

#endif //GRACEFUL_MUSIC_UTILS_H
