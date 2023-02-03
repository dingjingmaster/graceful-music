//
// Created by dingjing on 2/3/23.
//

#include "cache.h"

#include <glib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdatomic.h>

#include "utils.h"
#include "global.h"

#define CACHE_VERSION               0X0D
#define CACHE_64_BIT                0X01
#define CACHE_BE                    0x02
#define CACHE_RESERVED_PATTERN      0XFF
#define CACHE_ENTRY_USED_SIZE       28
#define CACHE_ENTRY_RESERVED_SIZE   52
#define CACHE_ENTRY_TOTAL_SIZE      (CACHE_ENTRY_RESERVED_SIZE + CACHE_ENTRY_USED_SIZE)

#define HASH_SIZE                   1023
#define ALIGN(size)                 (((size) + sizeof(long) - 1) & ~(sizeof(long) - 1))

typedef struct _CacheEntry          CacheEntry;

struct _CacheEntry
{
    uint32_t                    size;
    int32_t                     playCount;
    int64_t                     mTime;
    int32_t                     duration;
    int32_t                     bitrate;
    int32_t                     bpm;

    uint8_t                     reserved[CACHE_ENTRY_RESERVED_SIZE];

    char                        strings[];
};

struct _FifoWatcher
{
    FifoWatcher* _Atomic    next;
    pthread_cond_t          cond;
};

struct _CacheMutex
{
    FifoWatcher* _Atomic    tail;
    FifoWatcher*            head;
    pthread_mutex_t         mutex;
};

static char                     gCacheHeader[8] = "CTC\0\0\0\0\0";
static TrackInfo*               gHashTable[HASH_SIZE];
static char*                    gCacheFileName;
static int                      gTotal;

CacheMutex                      gCacheMutex = {.mutex = PTHREAD_MUTEX_INITIALIZER, .tail = ATOMIC_VAR_INIT(NULL)};

static int read_cache (void);
static TrackInfo* cache_entry_to_ti (CacheEntry* e);
static void add_ti (TrackInfo* ti, unsigned int hash);
static int valid_cache_entry (const CacheEntry* e, unsigned int avail);


int cache_init (void)
{
    unsigned int flags = 0;

    if (sizeof (long) == 8) {
        flags |= CACHE_64_BIT;
    }

    gCacheHeader[7] = flags & 0xFF;     flags >>= 8;
    gCacheHeader[6] = flags & 0xFF;     flags >>= 8;
    gCacheHeader[5] = flags & 0xFF;     flags >>= 8;
    gCacheHeader[4] = flags & 0xFF;

    // version
    gCacheHeader[3] = CACHE_VERSION;

    gCacheFileName = g_strdup_printf ("%s/cache", gMusicConfigDir);

    return read_cache ();
}

int cache_close (void)
{

}

TrackInfo*  cache_get_ti        (const char* fileName, int force);
void        cache_remove_ti     (TrackInfo* ti);
TrackInfo** cache_refresh       (int* count, int force);
TrackInfo*  lookup_cache_entry  (const char* fileName, unsigned int hash);

void cache_mutex_lock (CacheMutex* cacheMutex)
{
    FifoWatcher self = {
        .cond = PTHREAD_COND_INITIALIZER,
        .next = ATOMIC_VAR_INIT(NULL),
    };

    FifoWatcher* oldTail = atomic_exchange_explicit(&cacheMutex->tail, &self, memory_order_relaxed);
    if (oldTail) {
        atomic_store_explicit(&oldTail->next, &self, memory_order_release);
    }

    pthread_mutex_lock (&cacheMutex->mutex);
    if (oldTail) {
        while (cacheMutex->head != &self) {
            pthread_cond_wait (&self.cond, &cacheMutex->mutex);
        }
        pthread_cond_destroy (&self.cond);
    }

    FifoWatcher* selfAddr = &self;
    bool wasTail = atomic_compare_exchange_strong_explicit(&cacheMutex->tail, &selfAddr, NULL, memory_order_relaxed, memory_order_release);

    FifoWatcher* next = NULL;
    if (!wasTail) {
        while (!(next = atomic_load_explicit(&self.next, memory_order_consume)));
    }

    cacheMutex->head = next;
}

void cache_mutex_unlock (CacheMutex* cacheMutex)
{
    if (!cacheMutex->head) {
        pthread_cond_signal (&cacheMutex->head->cond);
    }

    pthread_mutex_unlock (&cacheMutex->mutex);
}

void cache_mutex_yield (CacheMutex* cacheMutex)
{
    if (cacheMutex->head || atomic_load_explicit(&cacheMutex->tail, memory_order_relaxed)) {
        cache_mutex_unlock (cacheMutex);
        cache_mutex_lock (cacheMutex);
    }
}

static int read_cache (void)
{
    char* buf = NULL;
    struct stat st = {};
    unsigned int size = 0, offset = 0;

    int fd = open (gCacheFileName, O_RDONLY);
    if (0 > fd) {
        if (ENOENT == errno) {
            return 0;
        }
        return -1;
    }

    fstat (fd, &st);
    if (sizeof (gCacheHeader) > st.st_size) {
        goto close;
    }
    size = st.st_size;

    buf = mmap (NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == buf) {
        close (fd);
        return -1;
    }

    if (0 != memcmp (buf, gCacheHeader, sizeof (gCacheHeader))) {
        goto corrupt;
    }

    offset = sizeof (gCacheHeader);
    while (offset < size) {
        CacheEntry* e = (void*) (buf + offset);
        TrackInfo* ti;

        if (!valid_cache_entry (e, size - offset)) {
            goto corrupt;
        }

        ti = cache_entry_to_ti (e);
        add_ti (ti, hash_str (ti->fileName));
        offset += ALIGN(e->size);
    }

corrupt:
    munmap (buf, size);

close:
    close (fd);

    return -2;
}

static int valid_cache_entry (const CacheEntry* e, unsigned int avail)
{
    int i = 0, count = 0;
    unsigned int strSize = 0;
    unsigned int minSize = sizeof (*e);

    if (avail < minSize) {
        return 0;
    }

    if (e->size < minSize || e->size > avail) {
        return 0;
    }

    strSize = e->size - minSize;
    count = 0;
    for (i = 0; i < strSize; ++i) {
        if (!e->strings[i]) {
            ++count;
        }
    }

    if (count % 2 == 0) {
        return 0;
    }

    if (e->strings[strSize - 1]) {
        return 0;
    }

    return 1;
}

static TrackInfo* cache_entry_to_ti (CacheEntry* e)
{
    const char* strings = e->strings;
    TrackInfo* ti = track_info_new (strings);
    KeyValue* kv = NULL;
    int pos = 0, i = 0, count = 0;
    int strSize = e->size - sizeof (*e);

    ti->duration = e->duration;
    ti->bitrate = e->bitrate;
    ti->mTime = e->mTime;
    ti->playCount = e->playCount;
    ti->bpm = e->bpm;

    count = 0;
    for (i = 0; i < strSize; ++i) {
        if (!strings[i]) {
            ++count;
        }
    }

    count = (count - 3) / 2;

    pos = (int) strlen (strings) + 1;
    ti->codec = strings[pos] ? g_strdup (strings + pos) : NULL;
    pos += (int) strlen (strings + pos) + 1;
    ti->codecProfile = strings[pos] ? g_strdup (strings + pos) : NULL;
    pos += (int) strlen (strings + pos) + 1;
    kv = (KeyValue*) g_malloc0 (sizeof(KeyValue) * (count + 1));
    for (i = 0; i < count; ++i) {
        int size = (int) strlen (strings + pos) + 1;
        kv[i].key = g_strdup (strings + pos);
        pos += size;

        size = (int) strlen (strings + pos) + 1;
        kv[i].value = g_strdup (strings + pos);
        pos += size;
    }

    kv[i].key = NULL;
    kv[i].value = NULL;
    track_info_set_comments (ti, kv);

    return ti;
}

static void add_ti (TrackInfo* ti, unsigned int hash)
{
    unsigned int pos = hash % HASH_SIZE;
    TrackInfo* next = gHashTable[pos];

    ti->next = next;
    gHashTable[pos] = ti;
    ++gTotal;
}