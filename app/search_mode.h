#ifndef CMUS_SEARCH_MODE_H
#define CMUS_SEARCH_MODE_H

#include "search.h"
#include "uchar.h"

#if defined(__sun__)
#include <ncurses.h>
#else
#include <curses.h>
#endif

extern char *search_str;
extern enum search_direction search_direction;

/* //WORDS or ??WORDS search mode */
extern int search_restricted;

void search_mode_ch(uchar ch);
void search_mode_escape(int c);
void search_mode_key(int key);
void search_mode_mouse(MEVENT *event);
void search_mode_init(void);
void search_mode_exit(void);

void search_text(const char *text, int restricted, int beginning);

#endif
