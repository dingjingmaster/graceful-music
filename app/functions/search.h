//
// Created by dingjing on 23-2-20.
//

#ifndef GRACEFUL_MUSIC_SEARCH_H
#define GRACEFUL_MUSIC_SEARCH_H
#include <glib.h>

typedef struct _Searchable                  Searchable;
typedef enum _SearchDirection               SearchDirection;
typedef struct _SearchableOptions           SearchableOptions;

enum _SearchDirection
{
    SEARCH_FORWARD,
    SEARCH_BACKWARD
};

struct _SearchableOptions
{
    int (*GetPrev)      (GList* l);
    int (*GetNext)      (GList* l);
    int (*GetCurrent)   (void* data, GList* l);
    int (*Matches)      (void* data, GList* l, const char* text);
};

Searchable* searchable_new      (void* data, const GList* head, const SearchableOptions* ops);
void        searchable_free     (Searchable* s);
void        searchable_set_head (Searchable* s, const GList* head);
int         search              (Searchable* s, const char* text, SearchDirection dir, int beginning);
int         search_next         (Searchable* s, const char* text, SearchDirection dir);

#endif //GRACEFUL_MUSIC_SEARCH_H
