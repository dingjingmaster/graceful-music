#ifndef CMUS_HISTORY_H
#define CMUS_HISTORY_H

#include "list.h"

struct history {
	struct list_head head;
	struct list_head *search_pos;
	char *filename;
	int max_lines;
	int lines;
};

void history_load(struct history *history, char *filename, int max_lines);
void history_save(struct history *history);
void history_free(struct history *history);
void history_add_line(struct history *history, const char *line);
void history_reset_search(struct history *history);
const char *history_search_forward(struct history *history, const char *text);
const char *history_search_backward(struct history *history, const char *text);

#endif
