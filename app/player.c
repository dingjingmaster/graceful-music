//
// Created by dingjing on 1/31/23.
//

#include "player.h"

#include <pthread.h>

#include "global.h"
#include "buffer.h"


#define PLAYER_LOCK() \
    do { \
		consumer_lock(); \
		producer_lock(); \
	} while (0)

#define PLAYER_UNLOCK() \
	do { \
		producer_unlock(); \
		consumer_unlock(); \
	} while (0)


const char* const gPlayerStatusNames[] =
{
    "stopped",
    "playing",
    "paused",
    NULL
};

enum _ProducerStatus
{
    PS_UNLOADED,
    PS_STOPPED,
    PS_PLAYING,
    PS_PAUSED
};

enum _ConsumerStatus
{
    CS_STOPPED,
    CS_PLAYING,
    CS_PAUSED
};

typedef enum _ProducerStatus        ProducerStatus;
typedef enum _ConsumerStatus        ConsumerStatus;

static void player_status_changed (void);
static inline unsigned int buffer_second_size (void);

static pthread_t            gProducerThread;
static pthread_mutex_t      gProducerMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t       gProducerPlaying = PTHREAD_COND_INITIALIZER;

static pthread_t            gConsumerThread;
static pthread_mutex_t      gConsumerMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t       gConsumerPlaying = PTHREAD_COND_INITIALIZER;

static int                  gProducerRunning = 1;
static int                  gConsumerRunning = 1;

static ConsumerStatus       gConsumerStatus = CS_STOPPED;
static ProducerStatus       gProducerStatus = PS_UNLOADED;

static unsigned long        gConsumerPos = 0;
static unsigned long        gScalePos;

static double               gReplayGainScale = 1.0;

static SampleFormat         gBufferSF;

void player_init(void)
{
    pthread_attr_t attr;
    pthread_attr_t* attrP = NULL;

    /**
     * 1  s  is 176400 B (0.168 MB)
     * 10 s  is 1.68 MB
     */
     gBufferNRChunks = 10 * 44100 * 16 / 8 * 2 / CHUNK_SIZE;

     buffer_init();

     int rc = pthread_attr_init (&attr);
     if (0 != rc) {
         // warning;
     }

     rc = pthread_attr_setschedpolicy (&attr, SCHED_RR);
     if (rc) {
         // "could not set real-time scheduling pripority: %s", strerror(rc);
     }
     else  {
         struct sched_param param;

         // "using real-time scheduling"
         param.sched_priority = sched_get_priority_max (SCHED_RR);
         // "setting priority to %d", param.sched_priority

         rc = pthread_attr_setschedparam (&attr, &param);
         if (0 != rc) {
             // warning;
         }
         attrP = &attr;
     }

     rc = pthread_create (&gProducerThread, NULL, producer_loop, NULL);
     if (0 != rc) {
         // error;
     }

     rc = pthread_create (&gConsumerThread, attrP, consumer_loop, NULL);
     if (rc && attrP) {
         // "could not create using real-time scheduling: %s", strerror(rc);
         rc = pthread_create (&gConsumerThread, NULL, consumer_loop, NULL);
     }

     if (0 != rc) {
         // error;
     }

     // 更新 player 信息
     PLAYER_LOCK();
     player_status_changed ();
     PLAYER_UNLOCK();
}

void player_exit(void)
{
    PLAYER_LOCK();
    gConsumerRunning = 0;
    pthread_cond_broadcast (&gConsumerPlaying);
    gProducerRunning = 0;
    pthread_cond_broadcast (&gProducerPlaying);
    PLAYER_UNLOCK();

    int rc = pthread_join (gConsumerThread, NULL);
    if (0 != rc) {
        // warning
    }
    rc = pthread_join (gProducerThread, NULL);
    if (0 != rc) {
        //
    }

    buffer_free();
}

static void player_status_changed (void)
{
    unsigned int pos = 0;

    if (gConsumerStatus == CS_PLAYING || gConsumerStatus == CS_PAUSED) {
        pos = gConsumerPos / buffer_second_size();
    }

    // FIXME:// 未完

}
static inline unsigned int buffer_second_size (void)
{
    return SF_GET_SECOND_SIZE(gBufferSF);
}

void player_set_file (TrackInfo* ti)
{
    PLAYER_LOCK();
    producer_set_file(ti);
    if (gProducerStatus == PS_UNLOADED) {
        consumer_stop();
        goto out;
    }

    /* PS_STOPPED */
    if (gConsumerStatus == CS_PLAYING || gConsumerStatus == CS_PAUSED) {
        op_drop();
        producer_play();
        if (gProducerStatus == PS_UNLOADED) {
            consumer_stop();
            goto out;
        }
        change_sf(1);
    }

out:
    player_status_changed();
    if (gProducerStatus == PS_PLAYING) {
        prebuffer();
    }
    PLAYER_UNLOCK();
}

void player_play_file (TrackInfo* ti)
{
    PLAYER_LOCK();

    producer_set_file(ti);
    if (gProducerStatus == PS_UNLOADED) {
        consumer_stop();
        goto out;
    }

    /* PS_STOPPED */
    producer_play();

    /* PS_UNLOADED,PS_PLAYING */
    if (gProducerStatus == PS_UNLOADED) {
        consumer_stop();
        goto out;
    }

    /* PS_PLAYING */
    if (gConsumerStatus == CS_STOPPED) {
        consumer_play();
        if (gConsumerStatus == CS_STOPPED) {
            producer_stop();
        }
    } else {
        op_drop();
        change_sf(1);
    }

out:
    player_status_changed();
    if (gProducerStatus == PS_PLAYING) {
        prebuffer();
    }
    PLAYER_UNLOCK();
}