//
// Created by dingjing on 1/31/23.
//

#include "buffer.h"

#include <stdlib.h>
#include <pthread.h>

#include "global.h"

/**
 * @brief
 *  chunk 仅仅可被 consumer 或 producer 二者之一访问(无须上锁)
 */
struct _Chunk
{
    char                data[CHUNK_SIZE];
    unsigned int        l;                          // index to data, first filled byte
    unsigned int        h : 31;                     // index to data, last filled byte + 1, there are h - l bytes available(filled)
    unsigned int        filled : 1;                 // if chunk is marked filled it can only be access by consumer otherwise only producer is allowed to access the chunk
};

typedef struct _Chunk       Chunk;


unsigned int            gBufferNRChunks = 0;
static pthread_mutex_t  gBufferMutex = PTHREAD_MUTEX_INITIALIZER;
static Chunk*           gBufferChunks = NULL;
static unsigned int     gBufferReadIdx;
static unsigned int     gBufferWriteIdx;

void buffer_init (void)
{
    free (gBufferChunks);
    gBufferChunks = (Chunk*) malloc (sizeof (Chunk) * gBufferNRChunks);
    buffer_reset();
}

void buffer_free (void)
{
    free (gBufferChunks);
}

int buffer_get_read_pos  (char** pos)
{
    int size = 0;

    int rc = pthread_mutex_lock (&gBufferMutex);
    if (0 != rc) {
        // error;
    }

    Chunk* c = &gBufferChunks[gBufferReadIdx];
    if (c->filled) {
        size = c->h - c->l;
        *pos = c->data + c->l;
    }

    rc = pthread_mutex_unlock (&gBufferMutex);
    if (0 != rc) {
        // error;
    }

    return size;
}

int buffer_get_write_pos (char** pos)
{
    int size = 0;

    int rc = pthread_mutex_lock (&gBufferMutex);
    if (0 != rc) {
        // error;
    }

    Chunk* c = &gBufferChunks[gBufferWriteIdx];
    if (!c->filled) {
        size = CHUNK_SIZE - c->h;
        *pos = c->data + c->h;
    }

    rc = pthread_mutex_unlock (&gBufferMutex);
    if (0 != rc) {
        // error;
    }

    return size;
}

void buffer_consume (int count)
{
    if (count < 0) {
        // warning;
    }

    int rc = pthread_mutex_lock (&gBufferMutex);
    if (0 != rc) {
        // error;
    }

    Chunk* c = &gBufferChunks[gBufferReadIdx];
    if (!c->filled) {
        // warning;
    }

    c->l += count;

    if (c->l == c->h) {
        c->l = 0;
        c->h = 0;
        c->filled = 0;
        ++gBufferReadIdx;
        gBufferReadIdx %= gBufferNRChunks;
    }

    rc = pthread_mutex_unlock (&gBufferMutex);
    if (0 != rc) {
        // error;
    }
}

int buffer_fill (int count)
{
    int filled = 0;

    int rc = pthread_mutex_lock (&gBufferMutex);
    if (0 != rc) {
        // error;
    }

    Chunk* c = &gBufferChunks[gBufferWriteIdx];
    if (c->filled) {
        // warning;
    }

    c->h += count;

    if (CHUNK_SIZE - c->h < 1024 || (count == 0 && c->h > 0)) {
        c->filled = 1;
        ++gBufferWriteIdx;
        gBufferWriteIdx %= gBufferNRChunks;
        filled = 1;
    }

    rc = pthread_mutex_unlock (&gBufferMutex);
    if (0 != rc) {
        // error;
    }

    return filled;
}

void buffer_reset (void)
{
    int rc = pthread_mutex_lock (&gBufferMutex);
    if (0 != rc) {
        // error;
    }

    gBufferReadIdx = 0;
    gBufferWriteIdx = 0;

    for (int i = 0; i < gBufferNRChunks; ++i) {
        gBufferChunks[i].l = 0;
        gBufferChunks[i].h = 0;
        gBufferChunks[i].filled = 0;
    }

    rc = pthread_mutex_unlock (&gBufferMutex);
    if (0 != rc) {
        // error;
    }
}

int buffer_get_filled_chunks (void)
{
    int c = 0;

    int rc = pthread_mutex_lock (&gBufferMutex);
    if (0 != rc) {
        // error;
    }

    if (gBufferReadIdx < gBufferWriteIdx) {
        /*
         * |__##########____|
         *    r         w
         *
         * |############____|
         *  r           w
         */
        c = gBufferWriteIdx - gBufferReadIdx;
    } else if (gBufferReadIdx > gBufferWriteIdx) {
        /*
         * |#######______###|
         *         w     r
         *
         * |_____________###|
         *  w            r
         */
        c = gBufferNRChunks - gBufferReadIdx + gBufferWriteIdx;
    } else {
        /*
         * |################|
         *     r
         *     w
         *
         * |________________|
         *     r
         *     w
         */
        if (gBufferChunks[gBufferReadIdx].filled) {
            c = gBufferNRChunks;
        } else {
            c = 0;
        }
    }

    rc = pthread_mutex_unlock (&gBufferMutex);
    if (0 != rc) {
        // error;
    }

    return c;
}