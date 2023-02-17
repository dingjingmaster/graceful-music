#ifndef CMUS_JOB_H
#define CMUS_JOB_H

#include "graceful-music.h"

#define JOB_TYPE_LIB   1 << 0
#define JOB_TYPE_PL    1 << 1
#define JOB_TYPE_QUEUE 1 << 2

#define JOB_TYPE_ADD          1 << 16
#define JOB_TYPE_UPDATE       1 << 17
#define JOB_TYPE_UPDATE_CACHE 1 << 18
#define JOB_TYPE_DELETE       1 << 19

struct add_data
{
	FileType type;
	char *name;
	add_ti_cb add;
	void *opaque;
	unsigned int force : 1;
};

struct update_data {
	size_t size;
	size_t used;
	struct track_info **ti;
	unsigned int force : 1;
};

struct update_cache_data {
	unsigned int force : 1;
};

struct pl_delete_data {
	struct playlist *pl;
	void (*cb)(struct playlist *);
};

extern int job_fd;

void job_init(void);
void job_exit(void);
void job_schedule_add(int type, struct add_data *data);
void job_schedule_update(struct update_data *data);
void job_schedule_update_cache(int type, struct update_cache_data *data);
void job_schedule_pl_delete(struct pl_delete_data *data);
void job_handle(void);

#endif
