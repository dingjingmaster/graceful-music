#ifndef CMUS_CACHE_H
#define CMUS_CACHE_H

#include "track_info.h"
#include "locking.h"

extern struct fifo_mutex cache_mutex;

#define cache_lock() fifo_mutex_lock(&cache_mutex)
#define cache_yield() fifo_mutex_yield(&cache_mutex)
#define cache_unlock() fifo_mutex_unlock(&cache_mutex)

int cache_init(void);
int cache_close(void);
struct track_info *cache_get_ti(const char *filename, int force);
void cache_remove_ti(struct track_info *ti);
struct track_info **cache_refresh(int *count, int force);
struct track_info *lookup_cache_entry(const char *filename, unsigned int hash);

#endif
