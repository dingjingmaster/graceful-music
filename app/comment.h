
#ifndef CMUS_COMMENT_H
#define CMUS_COMMENT_H

#include "key-value.h"

int track_is_compilation(const KeyValue* comments);
int track_is_va_compilation(const KeyValue* comments);

const char *comments_get_albumartist(const KeyValue* comments);
const char *comments_get_artistsort(const KeyValue* comments); /* can return NULL */

int comments_get_int(const KeyValue* comments, const char *key);
int comments_get_signed_int(const KeyValue* comments, const char *key, long int *ival);
double comments_get_double(const KeyValue* comments, const char *key);
int comments_get_date(const KeyValue* comments, const char *key);

int comments_add(GrowingKeyValues* c, const char *key, char *val);
int comments_add_const(GrowingKeyValues* c, const char *key, const char *val);

#endif
