//
// Created by dingjing on 23-2-21.
//

#include "widget.h"


static void selected_changed (Widget* win);
static int selectable (Widget* win, GList* l);


Widget* widget_new (int (*GetPrev)(GList *), int (*GetNext)(GList *))
{
    Widget* win = g_malloc0 (sizeof (Widget));

    win->GetPrev = GetPrev;
    win->GetNext = GetNext;
    win->Selectable = NULL;
    win->SelectChanged = NULL;
    win->rows = 1;
    win->changed = true;

    win->top = NULL;
    win->head = NULL;
    win->selected = NULL;

    return win;
}

void widget_free(Widget* win)
{
    if (win) {
        free (win);
    }
}

void widget_set_empty(Widget* win)
{
    if (!win) {
        return;
    }

    win->top = NULL;
    win->head = NULL;
    win->selected = NULL;

    selected_changed (win);
}

void widget_set_contents(Widget *win, void* head)
{
    GList* first = win->head->data = head;

    win->GetNext (&first);
    win->top = first;
    win->selected = first;

    while (!selectable (win, &win->selected)) {
        win->GetNext(&win->selected);
    }

    selected_changed (win);
}

void widget_changed (Widget* win)
{
    GList* iter;
    int delta = 0, rows = 0;

    if (!win->head) {
        // FIXME:// log
        return;
    }

    if (win->head == win->top) {
        win->GetNext (&win->top);
        win->selected = win->top;
        selected_changed (win);
        return;
    }

    iter = win->top;

    while (iter != win->selected) {
        if (!win->GetNext (iter)) {
            while (win->top != win->selected) {
                win->GetPrev (win->top);
            }
            goto minimize;
        }
        ++delta;
    }

    while (delta > win->rows - 1) {
        win->GetNext(win->top);
        --delta;
    }

minimize:
    iter = win->top;
    rows = 1;

    while (rows < win->rows) {
        if (!win->GetNext(iter)) {
            break;
        }
        ++rows;
    }

    while (rows < win->rows) {
        iter = win->top;
        if (!win->GetPrev(iter)) {
            break;
        }
        win->top = iter;
        ++rows;
    }
    win->changed = 1;
}

void widget_row_vanished(Widget* win, GList* iter)
{
    GList* new = iter;

    if (!win->GetNext(new) && !win->GetPrev(new)) {
        widget_set_empty(win);
    }

    if (win->top == iter) {
        new = *iter;
        if (win->GetNext (new)) {
            win->top = new;
        } else {
            new = iter;
            win->GetPrev(new);
            win->top = new;
        }
    }
    if (win->selected == iter) {
        /* calculate minimal distance to next selectable */
        int down = 0;
        int up = 0;
        new = iter;
        do {
            if (!win->GetNext(new)) {
                down = 0;
                break;
            }
            ++down;
        } while (!selectable(win, new));
        new = iter;
        do {
            if (!win->GetPrev(new)) {
                up = 0;
                break;
            }
            up++;
        } while (!selectable(win, new));
        new = iter;
        if (down > 0 && (up == 0 || down <= up)) {
            do {
                win->GetNext(new);
            } while (!selectable(win, new));
        }
        else if (up > 0) {
            do {
                win->GetPrev(new);
            } while (!selectable(win, &new));
        }
        win->selected = new;
        selected_changed(win);
    }

    win->changed = 1;
}

bool widget_get_top(Widget* win, GList** iter)
{
    *iter = win->top;

    return (win->top ? true: false);
}

bool widget_get_selected(Widget* win, GList** iter)
{
    *iter = win->selected;

    return (win->selected ? true : false);
}

bool widget_get_prev(Widget *win, GList **iter)
{
    return win->GetPrev(iter);
}

bool widget_get_next(Widget *win, GList **iter)
{
    return win->GetNext(iter);
}

void widget_set_selected(Widget* win, GList** iter)
{
    int sel_nr, top_nr, bottom_nr;
    int upper_bound;

}

void widget_set_nr_rows(Widget *win, int nrRows)
{

}

void widget_up(Widget *win, int rows)
{

}

void widget_down(Widget *win, int rows)
{

}

void widget_goto_top(Widget *win)
{

}

void widget_goto_bottom(Widget *win)
{

}

void widget_page_up(Widget *win)
{

}

void widget_half_page_up(Widget *win)
{

}

void widget_page_down(Widget *win)
{

}

void widget_half_page_down(Widget *win)
{

}

void widget_scroll_down(Widget *win)
{

}

void widget_scroll_up(Widget *win)
{

}

void widget_page_top(Widget *win)
{

}

void widget_page_bottom(Widget *win)
{

}

void widget_page_middle(Widget *win)
{

}

int widget_get_nr_rows(Widget *win)
{
    return 0;
}

static void selected_changed (Widget* win)
{
    if (win->SelectChanged) {
        win->SelectChanged();
    }

    win->changed = true;
}
