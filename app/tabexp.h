#ifndef CMUS_TABEXP_H
#define CMUS_TABEXP_H

struct tabexp {
	char *head;
	char **tails;
	int count;
};

extern struct tabexp tabexp;

/* return expanded src or NULL */
char *tabexp_expand(const char *src, void (*load_matches)(const char *src), int direction);

void tabexp_reset(void);

#endif
