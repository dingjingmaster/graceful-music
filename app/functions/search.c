//
// Created by dingjing on 23-2-20.
//

#include "search.h"

#include "../global.h"
#include "../convert.h"
#include "../options.h"
#include "../ui_curses.h"

struct _Searchable
{
    void*                       data;
    GList                       head;
    SearchableOptions           ops;
};

static int do_search(Searchable* s, GList* iter, const char *text, SearchDirection dir, int skipCurrent);


Searchable* searchable_new (void* data, const GList* head, const SearchableOptions* ops)
{
    Searchable* s = g_malloc0 (sizeof (Searchable));

    s->data = data;
    s->head = *head;
    s->ops = *ops;

    return s;
}

void searchable_free (Searchable* s)
{
    if (s) free (s);
}

void searchable_set_head (Searchable* s, const GList* head)
{
    if (s) {
        s->head = *head;
    }
}

int search (Searchable* s, const char* text, SearchDirection dir, int beginning)
{
    int ret = 0;
    GList it = {0};

    if (beginning) {
        it = s->head;
        if (SEARCH_FORWARD == dir) {
            ret = s->ops.GetNext (&it);
        }
        else {
            ret = s->ops.GetPrev (&it);
        }
    }
    else {
        ret = s->ops.GetCurrent (s->data, &it);
    }

    if (ret) {
        ret = do_search (s, &it, text, dir, 0);
    }

    return ret;
}

int search_next (Searchable* s, const char* text, SearchDirection dir)
{
    GList       it;
    int         ret = 0;

    if (!s->ops.GetCurrent(s->data, &it)) {
        return 0;
    }

    ret = do_search (s, &it, text, dir, 1);

    return ret;
}




static int advance (Searchable* s, GList* iter, SearchDirection dir, int* wrapped)
{
    if (dir == SEARCH_FORWARD) {
        if (!s->ops.GetNext(iter)) {
            if (!wrap_search) {
                return 0;
            }
            *iter = s->head;
            if (!s->ops.GetNext(iter)) {
                return 0;
            }
            *wrapped = 1;
        }
    }
    else {
        if (!s->ops.GetPrev (iter)) {
            if (!wrap_search) {
                return 0;
            }
            *iter = s->head;
            if (!s->ops.GetPrev (iter)) {
                return 0;
            }
            *wrapped = 1;
        }
    }
    return 1;
}

/* returns next matching item or NULL if not found
 * result can be the current item unless skip_current is set */
static int do_u_search (Searchable* s, GList* iter, const char* text, SearchDirection dir, int skipCurrent)
{
    GList start;
    int wrapped = 0;

    if (skipCurrent && !advance(s, iter, dir, &wrapped)) {
        return 0;
    }

    start = *iter;
    while (1) {
        if (s->ops.Matches(s->data, iter, text)) {
            if (wrapped) {
                info_msg(dir == SEARCH_FORWARD ? "search hit BOTTOM, continuing at TOP" : "search hit TOP, continuing at BOTTOM");
            }
            return 1;
        }
        if (!advance(s, iter, dir, &wrapped) || iters_equal(iter, &start))
            return 0;
    }
}

static int do_search (Searchable* s, GList* iter, const char *text, SearchDirection dir, int skipCurrent)
{
    char* uText = NULL;

    /* search text is always in locale encoding (because cmdline is) */
    if (!gUsingUtf8 && utf8_encode(text, gCharset, &uText) == 0) {
        text = uText;
    }

    int r = do_u_search(s, iter, text, dir, skipCurrent);

    free (uText);

    return r;
}