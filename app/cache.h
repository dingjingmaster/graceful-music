//
// Created by dingjing on 2/3/23.
//

#ifndef GRACEFUL_MUSIC_CACHE_H
#define GRACEFUL_MUSIC_CACHE_H
#include "track-info.h"

typedef struct _CacheMutex          CacheMutex;
typedef struct _FifoWatcher         FifoWatcher;

int         cache_init          (void);
int         cache_close         (void);
TrackInfo*  cache_get_ti        (const char* fileName, int force);
void        cache_remove_ti     (TrackInfo* ti);
TrackInfo** cache_refresh       (int* count, int force);
TrackInfo*  lookup_cache_entry  (const char* fileName, unsigned int hash);

void        cache_mutex_lock    (CacheMutex* cacheMutex);
void        cache_mutex_unlock  (CacheMutex* cacheMutex);
void        cache_mutex_yield   (CacheMutex* cacheMutex);


#endif //GRACEFUL_MUSIC_CACHE_H
