#ifndef CMUS_TABEXP_FILE_H
#define CMUS_TABEXP_FILE_H

#include <sys/stat.h>

void expand_files_and_dirs(const char *src,
		int (*filter)(const char *name, const struct stat *s));
void expand_env_path(const char *src,
		int (*filter)(const char *name, const struct stat *s));

#endif
