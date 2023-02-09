#ifndef CMUS_SEARCH_H
#define CMUS_SEARCH_H

#include "iter.h"

enum search_direction { SEARCH_FORWARD, SEARCH_BACKWARD };

struct searchable_ops {
	int (*get_prev)(struct iter *iter);
	int (*get_next)(struct iter *iter);
	int (*get_current)(void *data, struct iter *iter);
	int (*matches)(void *data, struct iter *iter, const char *text);
};

struct searchable;

struct searchable *searchable_new(void *data, const struct iter *head, const struct searchable_ops *ops);
void searchable_free(struct searchable *s);
void searchable_set_head(struct searchable *s, const struct iter *head);

int search(struct searchable *s, const char *text, enum search_direction dir, int beginning);
int search_next(struct searchable *s, const char *text, enum search_direction dir);

#endif
