
#ifndef CMUS_COMMENT_H
#define CMUS_COMMENT_H

#include "keyval.h"

int track_is_compilation(const struct keyval *comments);
int track_is_va_compilation(const struct keyval *comments);

const char *comments_get_albumartist(const struct keyval *comments);
const char *comments_get_artistsort(const struct keyval *comments); /* can return NULL */

int comments_get_int(const struct keyval *comments, const char *key);
int comments_get_signed_int(const struct keyval *comments, const char *key, long int *ival);
double comments_get_double(const struct keyval *comments, const char *key);
int comments_get_date(const struct keyval *comments, const char *key);

int comments_add(struct growing_keyvals *c, const char *key, char *val);
int comments_add_const(struct growing_keyvals *c, const char *key, const char *val);

#endif
