#ifndef CMUS_PLAY_QUEUE_H
#define CMUS_PLAY_QUEUE_H

#include "editable.h"
#include "track_info.h"

extern struct editable pq_editable;

void play_queue_init(void);
void play_queue_append(struct track_info *ti, void *opaque);
void play_queue_prepend(struct track_info *ti, void *opaque);
struct track_info *play_queue_remove(void);
int play_queue_for_each(int (*cb)(void *data, struct track_info *ti),
		void *data, void *opaque);

#endif
