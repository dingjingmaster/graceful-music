//
// Created by dingjing on 2/13/23.
//

#include "utils.h"

#include <gio/gio.h>

static inline GFile* get_file_by_path (const char* path)
{
    if (!path) {
        return NULL;
    }

    if (g_str_has_prefix (path, "file://")) {
        return g_file_new_for_uri (path);
    }
    else if (g_str_has_prefix (path, "/") || g_str_has_prefix (path, "~")) {
        return g_file_new_for_path (path);
    }
    else {
        g_autofree char* absPath = g_canonicalize_filename (path, g_get_current_dir());
        return g_file_new_for_path (absPath);
    }
}

bool path_is_dir(const char* path)
{
    g_autoptr(GFile)    file = get_file_by_path (path);

    if (G_IS_FILE(file)) {
        GFileType fileType = g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL);
        return G_FILE_TYPE_DIRECTORY == fileType;
    }

    return false;
}

bool path_is_exist(const char *path)
{
    g_autoptr(GFile)    file = get_file_by_path (path);

    if (G_IS_FILE(file)) {
        return g_file_query_exists (G_FILE(file), NULL);
    }

    return false;
}
