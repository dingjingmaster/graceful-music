//
// Created by dingjing on 23-2-17.
//

#ifndef GRACEFUL_MUSIC_LOCKING_H
#define GRACEFUL_MUSIC_LOCKING_H

#include <pthread.h>
#include <stdatomic.h>

struct fifo_mutex
{
    struct fifo_waiter * _Atomic tail;
    struct fifo_waiter *head;
    pthread_mutex_t mutex;
};

extern pthread_t gMainThread;

#define GM_MUTEX_INITIALIZER        PTHREAD_MUTEX_INITIALIZER
#define GM_COND_INITIALIZER         PTHREAD_COND_INITIALIZER
#define GM_RWLOCK_INITIALIZER       PTHREAD_RWLOCK_INITIALIZER

#define FIFO_MUTEX_INITIALIZER { \
		.mutex = PTHREAD_MUTEX_INITIALIZER, \
		.tail = ATOMIC_VAR_INIT(NULL), \
	}

void gm_mutex_lock      (pthread_mutex_t *mutex);
void gm_mutex_unlock    (pthread_mutex_t *mutex);
void gm_rwlock_rdlock   (pthread_rwlock_t *lock);
void gm_rwlock_wrlock   (pthread_rwlock_t *lock);
void gm_rwlock_unlock   (pthread_rwlock_t *lock);

void fifo_mutex_lock    (struct fifo_mutex *fifo);
void fifo_mutex_unlock  (struct fifo_mutex *fifo);
void fifo_mutex_yield   (struct fifo_mutex *fifo);


#endif //GRACEFUL_MUSIC_LOCKING_H
