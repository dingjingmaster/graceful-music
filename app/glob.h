#ifndef CMUS_GLOB_H
#define CMUS_GLOB_H

#include "list.h"

void glob_compile(struct list_head *head, const char *pattern);
void glob_free(struct list_head *head);
int glob_match(struct list_head *head, const char *text);

#endif
