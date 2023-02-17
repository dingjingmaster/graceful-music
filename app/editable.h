#ifndef CMUS_EDITABLE_H
#define CMUS_EDITABLE_H

#include "list.h"
#include "track.h"
#include "rbtree.h"
#include "window.h"
#include "locking.h"
#include "graceful-music.h"

struct editable;

typedef void (*editable_free_track)(struct editable *e, struct list_head *head);

struct editable_shared {
	struct editable *owner;

	struct window *win;
	sort_key_t *sort_keys;
	char sort_str[128];
	editable_free_track free_track;
	struct searchable *searchable;
};

struct editable {
	struct list_head head;
	struct rb_root tree_root;
	unsigned int nr_tracks;
	unsigned int nr_marked;
	unsigned int total_time;
	struct editable_shared *shared;
};

void editable_shared_init(struct editable_shared *shared,
		editable_free_track free_track);
void editable_shared_set_sort_keys(struct editable_shared *shared,
		sort_key_t *keys);

void editable_init(struct editable *e, struct editable_shared *shared,
		int take_ownership);
void editable_take_ownership(struct editable *e);
void editable_add(struct editable *e, struct simple_track *track);
void editable_add_before(struct editable *e, struct simple_track *track);
void editable_remove_track(struct editable *e, struct simple_track *track);
void editable_remove_sel(struct editable *e);
void editable_sort(struct editable *e);
void editable_rand(struct editable *e);
void editable_toggle_mark(struct editable *e);
void editable_move_after(struct editable *e);
void editable_move_before(struct editable *e);
void editable_clear(struct editable *e);
void editable_remove_matching_tracks(struct editable *e,
		int (*cb)(void *data, struct track_info *ti), void *data);
void editable_mark(struct editable *e, const char *filter);
void editable_unmark(struct editable *e);
void editable_invert_marks(struct editable *e);
int _editable_for_each_sel(struct editable *e, track_info_cb cb, void *data,
		int reverse);
int editable_for_each_sel(struct editable *e, track_info_cb cb, void *data,
		int reverse, int advance);
int editable_for_each(struct editable *e, track_info_cb cb, void *data,
		int reverse);
void editable_update_track(struct editable *e, struct track_info *old, struct track_info *new);
int editable_empty(struct editable *e);

static inline void editable_track_to_iter(struct editable *e, struct simple_track *track, struct iter *iter)
{
	iter->data0 = &e->head;
	iter->data1 = track;
	iter->data2 = NULL;
}

#endif
