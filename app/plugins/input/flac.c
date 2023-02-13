//
// Created by dingjing on 2/10/23.
//
#include "flac.h"

#include "sf.h"
#include "log.h"
#include "utils.h"
#include "debug.h"
#include "xmalloc.h"
#include "interface.h"
#include "../comment.h"

#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#include <FLAC/export.h>
#include <FLAC/metadata.h>
#include <FLAC/stream_decoder.h>

#ifndef UINT64_MAX
#define UINT64_MAX                      ((uint64_t) - 1)
#endif

#define FLAC_MAX_CHANNELS               8

/* Reduce typing.  Namespaces are nice but FLAC API is fscking ridiculous.  */

/* functions, types, enums */
#define F(s)                            FLAC__stream_decoder_ ## s
#define T(s)                            FLAC__StreamDecoder ## s
#define Dec                             FLAC__StreamDecoder
#define E(s)                            FLAC__STREAM_DECODER_ ## s

typedef struct _FlacPrivate             FlacPrivate;


struct _FlacPrivate
{
    /* file/stream position and length */
    uint64_t                            pos;
    uint64_t                            len;

    Dec*                                dec;

    /* PCM data */
    char*                               buf;
    unsigned int                        bufSize;
    unsigned int                        bufWritePos;
    unsigned int                        bufReadPos;

    KeyValue*                           comments;
    double                              duration;
    long                                bitrate;
    int                                 bps;
};

static T(ReadStatus) read_cb (const Dec* dec, unsigned char* buf, size_t* size, void* data)
{
    (void) dec;
    InputPluginData* ipData = data;
    FlacPrivate* p = ipData->private;
    int rc;

    if (p->pos == p->len) {
        *size = 0;
        return E(READ_STATUS_END_OF_STREAM);
    }

    if (*size == 0) {
        return E(READ_STATUS_CONTINUE);
    }

    rc = (int) read (ipData->fd, buf, *size);
    if (rc == -1) {
        *size = 0;
        if (errno == EINTR || errno == EAGAIN) {
            /* FIXME: not sure how the flac decoder handles this */
            LOG_DEBUG ("interrupted");

            return E(READ_STATUS_CONTINUE);
        }
        return E(READ_STATUS_ABORT);
    }

    p->pos += rc;
    *size = rc;
    if (rc == 0) {
        /* should not happen */
        return E(READ_STATUS_END_OF_STREAM);
    }

    return E(READ_STATUS_CONTINUE);
}

static T(SeekStatus) seek_cb (const Dec* dec, uint64_t offset, void* data)
{
    (void) dec;
    InputPluginData* ipData = data;
    FlacPrivate* p = ipData->private;
    off_t off;

    if (p->len == UINT64_MAX) {
        return E(SEEK_STATUS_ERROR);
    }

    off = lseek (ipData->fd, (__off_t) offset, SEEK_SET);
    if (off == -1) {
        return E(SEEK_STATUS_ERROR);
    }

    p->pos = off;

    return E(SEEK_STATUS_OK);
}

static T(TellStatus) tell_cb (const Dec* dec, uint64_t* offset, void* data)
{
    (void) dec;
    InputPluginData* ipData = data;
    FlacPrivate* p = ipData->private;

    *offset = p->pos;
    return E(TELL_STATUS_OK);
}

static T(LengthStatus) length_cb (const Dec* dec, uint64_t* len, void* data)
{
    (void) dec;
    InputPluginData* ipData = data;
    FlacPrivate* p = ipData->private;

    if (ipData->remote) {
        return E(LENGTH_STATUS_ERROR);
    }
    *len = p->len;

    return E(LENGTH_STATUS_OK);
}

static int eof_cb (const Dec *dec, void *data)
{
    (void) dec;
    InputPluginData* ipData = data;
    FlacPrivate* p = ipData->private;

    return p->pos == p->len;;
}

#if defined(WORDS_BIGENDIAN)

#define LE32(x) swap_uint32(x)

#else

#define LE32(x)	(x)

#endif

static FLAC__StreamDecoderWriteStatus write_cb (const Dec* dec, const FLAC__Frame* frame, const int32_t* const* buf, void* data)
{
    (void) dec;
    InputPluginData* ip_data = data;
    FlacPrivate* p = ip_data->private;
    uint frames, bytes, size, channels, bits, depth;
    int ch, i = 0;
    uint nch;
    char *dest; int32_t src;

    frames = frame->header.blocksize;
    channels = sf_get_channels(ip_data->sf);
    bits = sf_get_bits(ip_data->sf);
    bytes = frames * bits / 8 * channels;
    size = p->bufSize;

    if (size - p->bufWritePos < bytes) {
        if (size < bytes)
            size = bytes;
        size *= 2;
        p->buf = xrenew(char, p->buf, size);
        p->bufSize = size;
    }

    depth = frame->header.bits_per_sample;
    if (!depth) {
        depth = p->bps;
    }
    nch = frame->header.channels;
    dest = p->buf + p->bufWritePos;
    for (i = 0; i < frames; i++) {
        for (ch = 0; ch < channels; ch++) {
            src = LE32(buf[ch % nch][i] << (bits - depth));
            memcpy(dest, &src, bits / 8);
            dest += bits / 8;
        }
    }

    p->bufWritePos += bytes;

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

/* You should make a copy of metadata with FLAC__metadata_object_clone() if you will
 * need it elsewhere. Since metadata blocks can potentially be large, by
 * default the decoder only calls the metadata callback for the STREAMINFO
 * block; you can instruct the decoder to pass or filter other blocks with
 * FLAC__stream_decoder_set_metadata_*() calls.
 */
static void metadata_cb (const Dec* dec, const FLAC__StreamMetadata* metadata, void* data)
{
    (void) dec;
    InputPluginData* ip_data = data;
    FlacPrivate* p = ip_data->private;

    switch (metadata->type) {
        case FLAC__METADATA_TYPE_STREAMINFO: {
            const FLAC__StreamMetadata_StreamInfo *si = &metadata->data.stream_info;
            int bits = 0;

            if (si->bits_per_sample >= 4 && si->bits_per_sample <= 32) {
                bits = p->bps = (int) si->bits_per_sample;
                bits = 8 * ((bits + 7) / 8);
            }

            ip_data->sf = sf_rate(si->sample_rate) | sf_bits(bits) | sf_signed(1) | sf_channels(si->channels);
            if (!ip_data->remote && si->total_samples) {
                p->duration = (double) si->total_samples / si->sample_rate;
                if (p->duration >= 1 && p->len >= 1) {
                    p->bitrate = p->len * 8 / p->duration;
                }
            }
            break;
        }
        case FLAC__METADATA_TYPE_VORBIS_COMMENT: {
            DEBUG ("VORBISCOMMENT");
            if (p->comments) {
                DEBUG ("Ignoring");
            } else {
                GROWING_KEY_VALUES(c);
                unsigned int nr;

                nr = metadata->data.vorbis_comment.num_comments;
                for (int i = 0; i < nr; i++) {
                    const char *str = (const char *) metadata->data.vorbis_comment.comments[i].entry;
                    char *key, *val;

                    val = strchr (str, '=');
                    if (!val)
                        continue;
                    key = xstrndup (str, val - str);
                    val = xstrdup (val + 1);
                    comments_add (&c, key, val);
                    free (key);
                }
                key_value_terminate (&c);
                p->comments = c.keyValues;
            }
            break;
        }
        default: {
            DEBUG ("something else");
            break;
        }
    }
}

static void error_cb (const Dec* dec, FLAC__StreamDecoderErrorStatus status, void* data)
{
    (void) dec;
    (void) data;
    ERROR ("FLAC error: %s", FLAC__StreamDecoderErrorStatusString[status]);
}

static void free_private (InputPluginData* ip_data)
{
    FlacPrivate* p = ip_data->private;
    int save = errno;

    F(finish)(p->dec);
    F(delete)(p->dec);
    if (p->comments) {
        key_value_free(p->comments);
    }
    free(p->buf);
    free(p);
    ip_data->private = NULL;
    errno = save;
}

/* http://flac.sourceforge.net/format.html#frame_header */
static void channel_map_init_flac(int channels, ChannelPosition* map)
{
    if (channels == 1) {
        map[0] = CHANNEL_POSITION_MONO;
    } else if (channels == 2) {
        map[0] = CHANNEL_POSITION_FRONT_LEFT;
        map[1] = CHANNEL_POSITION_FRONT_RIGHT;
    } else if (channels == 3) {
        map[0] = CHANNEL_POSITION_FRONT_LEFT;
        map[1] = CHANNEL_POSITION_FRONT_RIGHT;
        map[2] = CHANNEL_POSITION_FRONT_CENTER;
    } else if (channels == 4) {
        map[0] = CHANNEL_POSITION_FRONT_LEFT;
        map[1] = CHANNEL_POSITION_FRONT_RIGHT;
        map[2] = CHANNEL_POSITION_REAR_LEFT;
        map[3] = CHANNEL_POSITION_REAR_RIGHT;
    } else if (channels == 5) {
        map[0] = CHANNEL_POSITION_FRONT_LEFT;
        map[1] = CHANNEL_POSITION_FRONT_RIGHT;
        map[2] = CHANNEL_POSITION_FRONT_CENTER;
        map[3] = CHANNEL_POSITION_REAR_LEFT;
        map[4] = CHANNEL_POSITION_REAR_RIGHT;
    } else if (channels == 6) {
        map[0] = CHANNEL_POSITION_FRONT_LEFT;
        map[1] = CHANNEL_POSITION_FRONT_RIGHT;
        map[2] = CHANNEL_POSITION_FRONT_CENTER;
        map[3] = CHANNEL_POSITION_LFE;
        map[4] = CHANNEL_POSITION_REAR_LEFT;
        map[5] = CHANNEL_POSITION_REAR_RIGHT;
    } else if (channels == 7) {
        map[0] = CHANNEL_POSITION_FRONT_LEFT;
        map[1] = CHANNEL_POSITION_FRONT_RIGHT;
        map[2] = CHANNEL_POSITION_FRONT_CENTER;
        map[3] = CHANNEL_POSITION_LFE;
        map[4] = CHANNEL_POSITION_REAR_LEFT;
        map[5] = CHANNEL_POSITION_REAR_RIGHT;
        map[6] = CHANNEL_POSITION_REAR_CENTER;
    } else if (channels >= 8) {
        if (channels > 8) {
            d_print("Flac file with %d channels?!", channels);
        }
        map[0] = CHANNEL_POSITION_FRONT_LEFT;
        map[1] = CHANNEL_POSITION_FRONT_RIGHT;
        map[2] = CHANNEL_POSITION_FRONT_CENTER;
        map[3] = CHANNEL_POSITION_LFE;
        map[4] = CHANNEL_POSITION_REAR_LEFT;
        map[5] = CHANNEL_POSITION_REAR_RIGHT;
        map[6] = CHANNEL_POSITION_SIDE_LEFT;
        map[7] = CHANNEL_POSITION_SIDE_RIGHT;
    }
}

static int flac_open (InputPluginData* ip_data)
{
    FlacPrivate* p = NULL;

    Dec *dec = F(new)();

    const FlacPrivate privateInit = {
        .dec      = dec,
        .duration = -1,
        .bitrate  = -1,
        .bps      = 0
    };

    if (!dec)
        return -INPUT_ERROR_INTERNAL;

    p = (FlacPrivate*) xnew (FlacPrivate, 1);
    *p = privateInit;
    if (ip_data->remote) {
        p->len = UINT64_MAX;
    }
    else {
        off_t off = lseek(ip_data->fd, 0, SEEK_END);

        if (off == -1 || lseek(ip_data->fd, 0, SEEK_SET) == -1) {
            int save = errno;
            F(delete)(dec);
            free(p);
            errno = save;
            return -INPUT_ERROR_ERRNO;
        }
        p->len = off;
    }
    ip_data->private = p;

    FLAC__stream_decoder_set_metadata_respond_all(dec);
    if (FLAC__stream_decoder_init_stream(dec, read_cb, seek_cb, tell_cb, length_cb, eof_cb, write_cb, metadata_cb, error_cb, ip_data) != E(INIT_STATUS_OK)) {
        int save = errno;

        DEBUG ("init failed");
        F(delete)(p->dec);
        free(p);
        ip_data->private = NULL;
        errno = save;
        return -INPUT_ERROR_ERRNO;
    }

    ip_data->sf = 0;
    if (!F(process_until_end_of_metadata)(p->dec)) {
        free_private(ip_data);
        return -INPUT_ERROR_ERRNO;
    }

    if (!ip_data->sf) {
        free_private(ip_data);
        return -INPUT_ERROR_FILE_FORMAT;
    }
    int bits = sf_get_bits(ip_data->sf);
    if (!bits) {
        free_private(ip_data);
        return -INPUT_ERROR_SAMPLE_FORMAT;
    }

    int channels = sf_get_channels(ip_data->sf);
    if (channels > 8) {
        free_private(ip_data);
        return -INPUT_ERROR_FILE_FORMAT;
    }

    channel_map_init_flac(sf_get_channels(ip_data->sf), ip_data->channelMap);
    DEBUG ("sr: %d, ch: %d, bits: %d", sf_get_rate(ip_data->sf), channels, bits);

    return 0;
}

static int flac_close(InputPluginData* ip_data)
{
    free_private(ip_data);

    return 0;
}

static int flac_read (InputPluginData* ip_data, char *buffer, int count)
{
    FlacPrivate* p = ip_data->private;
    int avail;

    while (1) {
        avail = p->bufWritePos - p->bufReadPos;
        BUG_ON(avail < 0);
        if (avail > 0) {
            break;
        }
        FLAC__bool internal_error = !F(process_single)(p->dec);
        FLAC__StreamDecoderState state = F(get_state)(p->dec);
        if (state == E(END_OF_STREAM)) {
            return 0;
        }

        if (state == E(ABORTED) || state == E(OGG_ERROR) || internal_error) {
            DEBUG ("process_single failed");
            return -1;
        }
    }
    if (count > avail) {
        count = avail;
    }
    memcpy(buffer, p->buf + p->bufReadPos, count);
    p->bufReadPos += count;
    BUG_ON(p->bufReadPos > p->bufWritePos);
    if (p->bufReadPos == p->bufWritePos) {
        p->bufReadPos = 0;
        p->bufWritePos = 0;
    }


    return count;
}

/* Flush the input and seek to an absolute sample. Decoding will resume at the
 * given sample.
 */
static int flac_seek (InputPluginData* ip_data, double offset)
{
    FlacPrivate* p = ip_data->private;
    p->bufReadPos = 0;
    p->bufWritePos = 0;
    uint64_t sample;

    sample = (uint64_t)(offset * (double)sf_get_rate(ip_data->sf) + 0.5);
    if (!F(seek_absolute)(p->dec, sample)) {
        if (F(get_state(p->dec)) == FLAC__STREAM_DECODER_SEEK_ERROR) {
            if (!F(flush)(p->dec)) {
                DEBUG ("failed to flush");
            }
        }
        return -INPUT_ERROR_ERRNO;
    }

    return 0;
}

static int flac_read_comments (InputPluginData* ip_data, KeyValue** comments)
{
    FlacPrivate* p = ip_data->private;

    if (p->comments) {
        *comments = key_value_dup(p->comments);
    } else {
        *comments = key_value_new(0);
    }

    return 0;
}

static int flac_duration (InputPluginData* ip_data)
{
    FlacPrivate* p= ip_data->private;

    return (int) p->duration;
}

static long flac_bitrate (InputPluginData* ip_data)
{
    FlacPrivate* p= ip_data->private;

    return p->bitrate;
}

static char* flac_codec (InputPluginData* ipData)
{
    (void*) ipData;

    return xstrdup("flac");
}

static char* flac_codec_profile (InputPluginData* ipData)
{
    /* maybe identify compression-level over min/max blocksize/framesize */
    return NULL;
}


//
static const InputPluginOps  ops = {
    .Open = flac_open,
    .Close = flac_close,
    .Read = flac_read,
    .Seek = flac_seek,
    .ReadComments = flac_read_comments,
    .Duration = flac_duration,
    .Bitrate = flac_bitrate,
    .BitrateCurrent = flac_bitrate,
    .Codec = flac_codec,
    .CodecProfile = flac_codec_profile
};

static const int priority = 50;
static const char* name = "flac plugin";
static const char * const mimeTypes[] = { NULL };
static const InputPluginOpt options[] = { { NULL } };
static const unsigned int abiVersion = INPUT_ABI_VERSION;
static const char * const extensions[] = { "flac", "fla", NULL };

void flac_input_register (InputPlugin* plugin)
{
    plugin->ops = &ops;
    plugin->name = name;
    plugin->option = options;
    plugin->isExtension = false;
    plugin->priority = priority;
    plugin->mimeType = mimeTypes;
    plugin->extensions = extensions;
    plugin->abiVersion = abiVersion;
}
