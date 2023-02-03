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
