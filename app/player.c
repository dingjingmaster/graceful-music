//
// Created by dingjing on 1/31/23.
//

#include "player.h"

#include <pthread.h>

#include "input.h"
#include "global.h"
#include "buffer.h"

#define SOFT_VOL_SCALE 65536

#define CONSUMER_LOCK()         pthread_mutex_lock(&gConsumerMutex)
#define CONSUMER_UNLOCK()       pthread_mutex_unlock(&gConsumerMutex)

#define PRODUCER_LOCK()         pthread_mutex_lock(&gProducerMutex)
#define PRODUCER_UNLOCK()       pthread_mutex_unlock(&gProducerMutex)

#define PLAYER_LOCK() \
    do { \
		CONSUMER_LOCK(); \
		PRODUCER_LOCK(); \
	} while (0)

#define PLAYER_UNLOCK() \
	do { \
		PRODUCER_UNLOCK(); \
		CONSUMER_UNLOCK(); \
	} while (0)

#define SCALE_SAMPLES(TYPE, buffer, count, l, r, swap) \
{\
    const int frames = count / sizeof(TYPE) / 2; \
    TYPE *buf = (void *) buffer; \
    int i; \
    /* avoid under flowing -32768 to 32767 when scale is 65536 */ \
    if (l != SOFT_VOL_SCALE && r != SOFT_VOL_SCALE) { \
        for (i = 0; i < frames; i++) { \
            scale_sample_##TYPE(buf, i * 2, l, swap); \
            scale_sample_##TYPE(buf, i * 2 + 1, r, swap); \
        } \
    } else if (l != SOFT_VOL_SCALE) { \
        for (i = 0; i < frames; i++) { \
            scale_sample_##TYPE(buf, i * 2, l, swap); \
        } \
    } else if (r != SOFT_VOL_SCALE) { \
        for (i = 0; i < frames; i++) {\
            scale_sample_##TYPE(buf, i * 2 + 1, r, swap); \
        } \
    } \
}



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

/**
 * @brief
 *  continue playing after track is finished?
 */
int gPlayerContinue = 1;

/**
 * @brief
 *  continue playing after album is finished?
 */
int gPlayerContinueAlbum = 1;

/**
 * @brief
 *  repeat current track forever?
 */
int gPlayerRepeatCurrent = 0;

ReplayGain                  gReplayGain;

int                         gReplayGainLimit = 1;
double                      gReplayGainPreamp = 0.0;

int                         gSoftVol;
int                         gSoftVolL;
int                         gSoftVolR;

PlayerInfo                  gPlayerInfo;
static pthread_mutex_t      gPlayerInfoMutex = PTHREAD_MUTEX_INITIALIZER;

char gPlayerMetaData[255 * 16 + 1] = {0};

static PlayerInfo gPlayerInfoPrivate = {
    .ti = NULL,
    .status = PLAYER_STATUS_STOPPED,
    .pos = 0,
    .currentBitrate = -1,
    .bufferFill = 0,
    .bufferSize = 0,
    .errorMsg = NULL,
    .fileChanged = 0,
    .metaDataChanged = 0,
    .statusChanged = 0,
    .positionChanged = 0,
    .bufferFillChanged = 0,
};

/**
 * @brief
 *  coefficients for volumes 0..99, for 100 65536 is used
 *  data copied from alsa-lib src/pcm/pcm-soft-vol.c
 */
static const unsigned short gSoftVolDB[100] = {
    0x0000, 0x0110, 0x011c, 0x012f, 0x013d, 0x0152, 0x0161, 0x0179,
    0x018a, 0x01a5, 0x01c1, 0x01d5, 0x01f5, 0x020b, 0x022e, 0x0247,
    0x026e, 0x028a, 0x02b6, 0x02d5, 0x0306, 0x033a, 0x035f, 0x0399,
    0x03c2, 0x0403, 0x0431, 0x0479, 0x04ac, 0x04fd, 0x0553, 0x058f,
    0x05ef, 0x0633, 0x069e, 0x06ea, 0x0761, 0x07b5, 0x083a, 0x0898,
    0x092c, 0x09cb, 0x0a3a, 0x0aeb, 0x0b67, 0x0c2c, 0x0cb6, 0x0d92,
    0x0e2d, 0x0f21, 0x1027, 0x10de, 0x1202, 0x12cf, 0x1414, 0x14f8,
    0x1662, 0x1761, 0x18f5, 0x1a11, 0x1bd3, 0x1db4, 0x1f06, 0x211d,
    0x2297, 0x24ec, 0x2690, 0x292a, 0x2aff, 0x2de5, 0x30fe, 0x332b,
    0x369f, 0x390d, 0x3ce6, 0x3f9b, 0x43e6, 0x46eb, 0x4bb3, 0x4f11,
    0x5466, 0x5a18, 0x5e19, 0x6472, 0x68ea, 0x6ffd, 0x74f8, 0x7cdc,
    0x826a, 0x8b35, 0x9499, 0x9b35, 0xa5ad, 0xad0b, 0xb8b7, 0xc0ee,
    0xcdf1, 0xd71a, 0xe59c, 0xefd3
};

// FIXME://
static InputPlugin*         gIp = NULL;


static void reset_buffer (void);
static int change_sf (int drop);
static void set_buffer_sf (void);

static inline void file_changed (TrackInfo* ti);

static void producer_unload (void);
static void producer_set_file (TrackInfo* ti);
static void producer_status_update (ProducerStatus status);

static void player_status_changed (void);
static inline unsigned int buffer_second_size (void);

static void* consumer_loop (void* arg);
static void* producer_loop (void* arg);


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


// OK
static void producer_set_file (TrackInfo* ti)
{
    producer_unload();
    gIp = input_new(ti->fileName);
    producer_status_update(PS_STOPPED);
    file_changed(ti);
}

static void producer_unload (void)
{
    producer_stop();
    if (gProducerStatus == PS_STOPPED) {
        ip_delete(gIp);
        producer_status_update(PS_UNLOADED);
    }
}

// OK
static void producer_status_update (ProducerStatus status)
{
    gProducerStatus = status;
    pthread_cond_broadcast(&gProducerPlaying);
}

static inline void file_changed (TrackInfo* ti)
{
    pthread_mutex_lock (&gPlayerInfoMutex);
    if (gPlayerInfoPrivate.ti)
        track_info_unref(gPlayerInfoPrivate.ti);

    gPlayerInfoPrivate.ti = ti;
    update_rg_scale();
    player_metadata[0] = 0;
    gPlayerInfoPrivate.fileChanged = 1;
    pthread_mutex_unlock (&gPlayerInfoMutex);
}

static int change_sf (int drop)
{
    int oldSF = gBufferSF;
    CHANNEL_MAP(old_channel_map);
    channel_map_copy(old_channel_map, buffer_channel_map);

    set_buffer_sf();
    if (gBufferSF != oldSF || !channel_map_equal(buffer_channel_map, old_channel_map, SF_GET_CHANNELS(gBufferSF))) {
        /* reopen */
        int rc;

        if (drop) {
            op_drop();
        }

        op_close();
        rc = op_open(gBufferSF, buffer_channel_map);
        if (rc) {
            player_op_error(rc, "opening audio device");
            consumer_status_update(CS_STOPPED);
            producer_stop();
            return rc;
        }
    } else if (gConsumerStatus == CS_PAUSED) {
        op_drop();
        op_unpause();
    }
    consumer_status_update(CS_PLAYING);

    return 0;
}

static void set_buffer_sf(void)
{
    gBufferSF = ip_get_sf(ip);
    ip_get_channel_map(ip, buffer_channel_map);

    /* ip_read converts samples to this format */
    if (SF_GET_CHANNELS(gBufferSF) <= 2 && SF_GET_BITS(gBufferSF) <= 16) {
        gBufferSF &= SF_RATE_MASK;
        gBufferSF |= SF_CHANNELS(2) | SF_BITS(16) | SF_SIGNED(1);
        gBufferSF |= SF_HOST_ENDIAN();
        channel_map_init_stereo(buffer_channel_map);
    }
}

// OK
static void reset_buffer (void)
{
    buffer_reset ();
    gConsumerPos = 0;
    gScalePos = 0;
    pthread_cond_broadcast(&gProducerPlaying);
}

static void* consumer_loop (void* arg)
{
    while (1) {
        int rc, space;
        int size;
        char *rpos;

        consumer_lock();
        if (!gConsumerRunning)
            break;

        if (consumer_status == CS_PAUSED || consumer_status == CS_STOPPED) {
            pthread_cond_wait(&consumer_playing, &consumer_mutex);
            consumer_unlock();
            continue;
        }
        space = op_buffer_space();
        if (space < 0) {
            d_print("op_buffer_space returned %d %s\n", space,
                    space == -1 ? strerror(errno) : "");

            /* try to reopen */
            op_close();
            _consumer_status_update(CS_STOPPED);
            _consumer_play();

            consumer_unlock();
            continue;
        }
/* 		d_print("BS: %6d %3d\n", space, space * 1000 / (44100 * 2 * 2)); */

        while (1) {
            if (space == 0) {
                _consumer_position_update();
                consumer_unlock();
                ms_sleep(25);
                break;
            }
            size = buffer_get_rpos(&rpos);
            if (size == 0) {
                producer_lock();
                if (producer_status != PS_PLAYING) {
                    producer_unlock();
                    consumer_unlock();
                    break;
                }
                /* must recheck rpos */
                size = buffer_get_rpos(&rpos);
                if (size == 0) {
                    /* OK. now it's safe to check if we are at EOF */
                    if (ip_eof(ip)) {
                        /* EOF */
                        _consumer_handle_eof();
                        producer_unlock();
                        consumer_unlock();
                        break;
                    } else {
                        /* possible underrun */
                        producer_unlock();
                        _consumer_position_update();
                        consumer_unlock();
/* 						d_print("possible underrun\n"); */
                        ms_sleep(10);
                        break;
                    }
                }

                /* player_buffer and ip.eof were inconsistent */
                producer_unlock();
            }
            if (size > space)
                size = space;
            if (soft_vol || replaygain)
                scale_samples(rpos, (unsigned int *)&size);
            rc = op_write(rpos, size);
            if (rc < 0) {
                d_print("op_write returned %d %s\n", rc,
                        rc == -1 ? strerror(errno) : "");

                /* try to reopen */
                op_close();
                _consumer_status_update(CS_STOPPED);
                _consumer_play();

                consumer_unlock();
                break;
            }
            buffer_consume(rc);
            consumer_pos += rc;
            space -= rc;
        }
    }
    _consumer_stop();
    consumer_unlock();
    return NULL;
}

static void* producer_loop (void* arg)
{
    while (1) {
        /* number of chunks to fill
         * too big   => seeking is slow
         * too small => underruns?
         */
        const int chunks = 1;
        int size, nr_read, i;
        char *wpos;

        producer_lock();
        if (!producer_running)
            break;

        if (producer_status == PS_UNLOADED ||
            producer_status == PS_PAUSED ||
            producer_status == PS_STOPPED || ip_eof(ip)) {
            pthread_cond_wait(&producer_playing, &producer_mutex);
            producer_unlock();
            continue;
        }
        for (i = 0; ; i++) {
            size = buffer_get_wpos(&wpos);
            if (size == 0) {
                /* buffer is full */
                producer_unlock();
                ms_sleep(50);
                break;
            }
            nr_read = ip_read(ip, wpos, size);
            if (nr_read < 0) {
                if (nr_read != -1 || errno != EAGAIN) {
                    player_ip_error(nr_read, "reading file %s",
                                    ip_get_filename(ip));
                    /* ip_read sets eof */
                    nr_read = 0;
                } else {
                    producer_unlock();
                    ms_sleep(50);
                    break;
                }
            }
            if (ip_metadata_changed(ip))
                metadata_changed();

            /* buffer_fill with 0 count marks current chunk filled */
            buffer_fill(nr_read);
            if (nr_read == 0) {
                /* consumer handles EOF */
                producer_unlock();
                ms_sleep(50);
                break;
            }
            if (i == chunks) {
                producer_unlock();
                /* don't sleep! */
                break;
            }
        }
        _producer_buffer_fill_update();
    }
    _producer_unload();
    producer_unlock();
    return NULL;
}

