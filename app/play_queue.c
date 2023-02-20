#include "play_queue.h"
#include "editable.h"
#include "track.h"
#include "xmalloc.h"

struct editable pq_editable;
static EditableShared 					gPlayQueueEditableShared;

static void pq_free_track(struct editable *e, struct list_head *item)
{
	struct simple_track *track = to_simple_track(item);

	track_info_unref(track->info);
	free(track);
}

void play_queue_init(void)
{
	editable_shared_init(&gPlayQueueEditableShared, pq_free_track);
	editable_init(&pq_editable, &gPlayQueueEditableShared, 1);
}

void play_queue_append(struct track_info *ti, void *opaque)
{
	struct simple_track *t = simple_track_new(ti);

	editable_add(&pq_editable, t);
}

void play_queue_prepend(struct track_info *ti, void *opaque)
{
	struct simple_track *t = simple_track_new(ti);

	editable_add_before(&pq_editable, t);
}

struct track_info *play_queue_remove(void)
{
	struct track_info *info = NULL;

	if (!list_empty(&pq_editable.head)) {
		struct simple_track *t = to_simple_track(pq_editable.head.next);
		info = t->info;
		track_info_ref(info);
		editable_remove_track(&pq_editable, t);
	}

	return info;
}

int play_queue_for_each(int (*cb)(void *data, struct track_info *ti),
		void *data, void *opaque)
{
	struct simple_track *track;
	int rc = 0;

	list_for_each_entry(track, &pq_editable.head, node) {
		rc = cb(data, track->info);
		if (rc)
			break;
	}
	return rc;
}
