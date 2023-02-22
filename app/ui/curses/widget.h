//
// Created by dingjing on 23-2-21.
//

#ifndef GRACEFUL_MUSIC_WIDGET_H
#define GRACEFUL_MUSIC_WIDGET_H
#include <glib.h>
#include <stdbool.h>

typedef struct _Widget          Widget;


struct _Widget
{
    GList*                      head;           // 元素的头节点
    GList*                      top;            // 首个元素
    GList*                      selected;       // 选中的元素

    int                         rows;
    bool                        changed;

    int     (*GetPrev)          (GList** l);
    int     (*GetNext)          (GList** l);
    int     (*Selectable)       (GList* l);
    void    (*SelectChanged)    (void);
};

Widget*     widget_new              (int(*GetPrev)(GList**), int(*GetNext)(GList**));
void        widget_free             (Widget* win);
void        widget_set_empty        (Widget* win);
void        widget_set_contents     (Widget* win, void* head);
void        widget_changed          (Widget* win);
void        widget_row_vanished     (Widget* win, GList* l);
bool        widget_get_top          (Widget* win, GList** iter);
bool        widget_get_selected     (Widget* win, GList** iter);
bool        widget_get_prev         (Widget* win, GList** iter);
bool        widget_get_next         (Widget* win, GList** iter);
void        widget_set_selected     (Widget* win, GList** iter);
void        widget_set_nr_rows      (Widget* win, int nrRows);
void        widget_up               (Widget* win, int rows);
void        widget_down             (Widget* win, int rows);
void        widget_goto_top         (Widget* win);
void        widget_goto_bottom      (Widget* win);
void        widget_page_up          (Widget* win);
void        widget_half_page_up     (Widget* win);
void        widget_page_down        (Widget* win);
void        widget_half_page_down   (Widget* win);
void        widget_scroll_down      (Widget* win);
void        widget_scroll_up        (Widget* win);
void        widget_page_top         (Widget* win);
void        widget_page_bottom      (Widget* win);
void        widget_page_middle      (Widget* win);
int         widget_get_nr_rows      (Widget* win);

#endif //GRACEFUL_MUSIC_WIDGET_H
