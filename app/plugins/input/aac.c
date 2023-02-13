//
// Created by dingjing on 2/13/23.
//
#include "aac.h"

#include "sf.h"
#include "id3.h"
#include "log.h"
#include "debug.h"
#include "xmalloc.h"
#include "interface.h"
#include "../../comment.h"
#include "../read_wrapper.h"

#include <neaacdec.h>

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

/* FAAD_MIN_STREAMSIZE == 768, 6 == # of channels */
#define BUFFER_SIZE	            (FAAD_MIN_STREAMSIZE * 6 * 4)

typedef struct _AACPrivate      AACPrivate;

struct _AACPrivate
{
    char                        rBuf[BUFFER_SIZE];
    int                         rBufLen;
    int                         rBufPos;

    unsigned char               channels;
    unsigned long               sampleRate;
    long                        bitrate;
    int                         objectType;

    struct {
        unsigned long   samples;
        unsigned long   bytes;
    } current;

    char*                       overflowBuf;
    int                         overflowBufLen;

    NeAACDecHandle              decoder;	/* typedef void * */
};

static inline int buffer_length(const InputPluginData* ipData)
{
    AACPrivate* p = ipData->private;

    return p->rBufLen - p->rBufPos;
}

static inline void* buffer_data(const InputPluginData* ip_data)
{
    AACPrivate* p = ip_data->private;

    return p->rBuf + p->rBufPos;
}

static int buffer_fill(InputPluginData* ip_data)
{
    AACPrivate* p = ip_data->private;
    int32_t n;

    if (p->rBufPos > 0) {
        p->rBufLen = buffer_length(ip_data);
        memmove(p->rBuf, p->rBuf + p->rBufPos, p->rBufLen);
        p->rBufPos = 0;
    }

    if (p->rBufLen == BUFFER_SIZE)
        return 1;

    n = read_wrapper(ip_data, p->rBuf + p->rBufLen, BUFFER_SIZE - p->rBufLen);
    if (n == -1)
        return -1;
    if (n == 0)
        return 0;

    p->rBufLen += n;

    return 1;
}

static inline void buffer_consume(InputPluginData* ip_data, int n)
{
    AACPrivate* priv = ip_data->private;

    BUG_ON(n > buffer_length(ip_data));

    priv->rBufPos += n;
}

static int buffer_fill_min(InputPluginData* ip_data, int len)
{
    int rc;

    BUG_ON(len > BUFFER_SIZE);

    while (buffer_length(ip_data) < len) {
        rc = buffer_fill(ip_data);
        if (rc <= 0)
            return rc;
    }
    return 1;
}

/* 'data' must point to at least 6 bytes of data */
static inline int parse_frame(const unsigned char data[6])
{
    int len;

    /* http://www.audiocoding.com/modules/wiki/?page=ADTS */

    /* first 12 bits must be set */
    if (data[0] != 0xFF)
        return 0;
    if ((data[1] & 0xF0) != 0xF0)
        return 0;

    /* layer is always '00' */
    if ((data[1] & 0x06) != 0x00)
        return 0;

    /* frame length is stored in 13 bits */
    len  = data[3] << 11;	/* ..1100000000000 */
    len |= data[4] << 3;	/* ..xx11111111xxx */
    len |= data[5] >> 5;	/* ..xxxxxxxxxx111 */
    len &= 0x1FFF;		/* 13 bits */

    return len;
}

/* scans forward to the next aac frame and makes sure
 * the entire frame is in the buffer.
 */
static int buffer_fill_frame(InputPluginData* ip_data)
{
    unsigned char *data;
    int rc, n, len;
    int max = 32768;

    while (1) {
        /* need at least 6 bytes of data */
        rc = buffer_fill_min(ip_data, 6);
        if (rc <= 0)
            return rc;

        len = buffer_length(ip_data);
        data = buffer_data(ip_data);

        /* scan for a frame */
        for (n = 0; n < len - 5; n++) {
            /* give up after 32KB */
            if (max-- == 0) {
                DEBUG ("no frame found!");
                /* FIXME: set errno? */
                return -1;
            }

            /* see if there's a frame at this location */
            rc = parse_frame(data + n);
            if (rc == 0)
                continue;

            /* found a frame, consume all data up to the frame */
            buffer_consume(ip_data, n);

            /* rc == frame length */
            rc = buffer_fill_min(ip_data, rc);
            if (rc <= 0)
                return rc;

            return 1;
        }

        /* consume what we used */
        buffer_consume(ip_data, n);
    }
    /* not reached */
}

static void aac_get_channel_map(InputPluginData* ip_data)
{
    AACPrivate* priv = ip_data->private;
    NeAACDecFrameInfo frame_info;
    void *buf;
    int i;

    ip_data->channelMap[0] = CHANNEL_POSITION_INVALID;

    if (buffer_fill_frame(ip_data) <= 0)
        return;

    buf = NeAACDecDecode(priv->decoder, &frame_info, buffer_data(ip_data), buffer_length(ip_data));
    if (!buf || frame_info.error != 0 || frame_info.bytesconsumed <= 0
        || frame_info.channels > CHANNELS_MAX)
        return;

    for (i = 0; i < frame_info.channels; i++)
        ip_data->channelMap[i] = channel_position_aac(frame_info.channel_position[i]);
}

static int aac_open(InputPluginData* ip_data)
{
    AACPrivate* priv;
    NeAACDecConfigurationPtr neaac_cfg;
    int ret, n;

    /* init private struct */
    const AACPrivate priv_init = {
        .decoder = NeAACDecOpen(),
        .bitrate = -1,
        .objectType = -1
    };
    priv = xnew (AACPrivate, 1);
    *priv = priv_init;
    ip_data->private = priv;

    /* set decoder config */
    neaac_cfg = NeAACDecGetCurrentConfiguration(priv->decoder);
    neaac_cfg->outputFormat = FAAD_FMT_16BIT;	/* force 16 bit audio */
    neaac_cfg->downMatrix = 0;			/* NOT 5.1 -> stereo */
    neaac_cfg->dontUpSampleImplicitSBR = 0;		/* upsample, please! */
    NeAACDecSetConfiguration(priv->decoder, neaac_cfg);

    /* find a frame */
    if (buffer_fill_frame(ip_data) <= 0) {
        ret = -INPUT_ERROR_FILE_FORMAT;
        goto out;
    }

    /* in case of a bug, make sure there is at least some data
     * in the buffer for NeAACDecInit() to work with.
     */
    if (buffer_fill_min(ip_data, 256) <= 0) {
        DEBUG ("not enough data\n");
        ret = -INPUT_ERROR_FILE_FORMAT;
        goto out;
    }

    /* init decoder, returns the length of the header (if any) */
    n = NeAACDecInit (priv->decoder, buffer_data(ip_data), buffer_length(ip_data), &priv->sampleRate, &priv->channels);
    if (n < 0) {
        DEBUG ("NeAACDecInit failed");
        ret = -INPUT_ERROR_FILE_FORMAT;
        goto out;
    }

    DEBUG ("sample rate %luhz, channels %u", priv->sampleRate, priv->channels);
    if (!priv->sampleRate || !priv->channels) {
        ret = -INPUT_ERROR_FILE_FORMAT;
        goto out;
    }

    /* skip the header */
    DEBUG ("skipping header (%d bytes)\n", n);

    buffer_consume(ip_data, n);

    /*NeAACDecInitDRM(priv->decoder, priv->sampleRate, priv->channels);*/

    ip_data->sf = sf_rate(priv->sampleRate) | sf_channels(priv->channels) | sf_bits(16) | sf_signed(1);
    ip_data->sf |= sf_host_endian();
    aac_get_channel_map(ip_data);

    return 0;
out:
    NeAACDecClose(priv->decoder);
    free(priv);
    return ret;
}

static int aac_close(InputPluginData* ip_data)
{
    AACPrivate* priv = ip_data->private;

    NeAACDecClose(priv->decoder);
    free(priv);
    ip_data->private = NULL;
    return 0;
}

/* returns -1 on fatal errors
 * returns -2 on non-fatal errors
 * 0 on eof
 * number of bytes put in 'buffer' on success */
static int decode_one_frame(InputPluginData* ip_data, void *buffer, int count)
{
    AACPrivate* priv = ip_data->private;
    unsigned char *aac_data;
    unsigned int aac_data_size;
    NeAACDecFrameInfo frame_info;
    char *sample_buf;
    int bytes, rc;

    rc = buffer_fill_frame(ip_data);
    if (rc <= 0)
        return rc;

    aac_data = buffer_data(ip_data);
    aac_data_size = buffer_length(ip_data);

    /* aac data -> raw pcm */
    sample_buf = NeAACDecDecode(priv->decoder, &frame_info, aac_data, aac_data_size);
    if (frame_info.error == 0 && frame_info.samples > 0) {
        priv->current.samples += frame_info.samples;
        priv->current.bytes += frame_info.bytesconsumed;
    }

    buffer_consume(ip_data, frame_info.bytesconsumed);

    if (!sample_buf || frame_info.bytesconsumed <= 0) {
        DEBUG ("fatal error: %s\n", NeAACDecGetErrorMessage(frame_info.error));
        errno = EINVAL;
        return -1;
    }

    if (frame_info.error != 0) {
        DEBUG ("frame error: %s\n", NeAACDecGetErrorMessage(frame_info.error));
        return -2;
    }

    if (frame_info.samples <= 0)
        return -2;

    if (frame_info.channels != priv->channels || frame_info.samplerate != priv->sampleRate) {
        DEBUG ("invalid channel or sample_rate count\n");
        return -2;
    }

    /* 16-bit samples */
    bytes = frame_info.samples * 2;

    if (bytes > count) {
        /* decoded too much, keep overflow */
        priv->overflowBuf = sample_buf + count;
        priv->overflowBufLen = bytes - count;
        memcpy(buffer, sample_buf, count);
        return count;
    } else {
        memcpy(buffer, sample_buf, bytes);
    }
    return bytes;
}

static int aac_read(InputPluginData* ip_data, char *buffer, int count)
{
    AACPrivate* priv = ip_data->private;
    int rc;

    /* use overflow from previous call (if any) */
    if (priv->overflowBufLen) {
        int len = priv->overflowBufLen;

        if (len > count)
            len = count;

        memcpy(buffer, priv->overflowBuf, len);
        priv->overflowBuf += len;
        priv->overflowBufLen -= len;
        return len;
    }

    do {
        rc = decode_one_frame(ip_data, buffer, count);
    } while (rc == -2);
    return rc;
}

static int aac_seek(InputPluginData* ip_data, double offset)
{
    return -INPUT_ERROR_FUNCTION_NOT_SUPPORTED;
}

static int aac_read_comments(InputPluginData* ip_data, KeyValue** comments)
{
    GROWING_KEY_VALUES(c);

    Id3Tag id3;
    int rc, fd, i;

    fd = open(ip_data->fileName, O_RDONLY);
    if (fd == -1) {
        return -1;
    }

    id3_init(&id3);
    rc = id3_read_tags(&id3, fd, ID3_V1 | ID3_V2);
    if (rc == -1) {
        DEBUG ("error: %s", strerror(errno));
        goto out;
    }

    for (i = 0; i < NUM_ID3_KEYS; i++) {
        char *val = id3_get_comment(&id3, i);

        if (val) {
            comments_add(&c, gId3KeyNames[i], val);
        }
    }
out:
    close(fd);
    id3_free(&id3);
    key_value_terminate (&c);
    *comments = c.keyValues;
    return 0;
}

static int aac_duration(InputPluginData* ip_data)
{
    AACPrivate* priv = ip_data->private;
    NeAACDecFrameInfo frame_info;
    int samples = 0, bytes = 0, frames = 0;
    off_t file_size;

    file_size = lseek(ip_data->fd, 0, SEEK_END);
    if (file_size == -1)
        return -INPUT_ERROR_FUNCTION_NOT_SUPPORTED;

    /* Seek to the middle of the file. There is almost always silence at
     * the beginning, which gives wrong results. */
    if (lseek(ip_data->fd, file_size/2, SEEK_SET) == -1)
        return -INPUT_ERROR_FUNCTION_NOT_SUPPORTED;

    priv->rBufPos = 0;
    priv->rBufLen = 0;

    /* guess track length by decoding the first 10 frames */
    while (frames < 10) {
        if (buffer_fill_frame(ip_data) <= 0)
            break;

        NeAACDecDecode(priv->decoder, &frame_info,
                       buffer_data(ip_data), buffer_length(ip_data));
        if (frame_info.error == 0 && frame_info.samples > 0) {
            samples += frame_info.samples;
            bytes += frame_info.bytesconsumed;
            frames++;
        }
        if (frame_info.bytesconsumed == 0)
            break;

        buffer_consume(ip_data, frame_info.bytesconsumed);
    }

    if (frames == 0)
        return -INPUT_ERROR_FUNCTION_NOT_SUPPORTED;

    samples /= frames;
    samples /= priv->channels;
    bytes /= frames;

    /*  8 * file_size / duration */
    priv->bitrate = (8 * bytes * priv->sampleRate) / samples;

    priv->objectType = frame_info.object_type;

    return ((file_size / bytes) * samples) / priv->sampleRate;
}

static long aac_bitrate(InputPluginData* ip_data)
{
    AACPrivate* priv = ip_data->private;
    return priv->bitrate != -1 ? priv->bitrate : -INPUT_ERROR_FUNCTION_NOT_SUPPORTED;
}

static long aac_current_bitrate(InputPluginData* ip_data)
{
    AACPrivate* priv = ip_data->private;
    long bitrate = -1;
    if (priv->current.samples > 0) {
        priv->current.samples /= priv->channels;
        bitrate = (8 * priv->current.bytes * priv->sampleRate) / priv->current.samples;
        priv->current.samples = 0;
        priv->current.bytes = 0;
    }
    return bitrate;
}

static char *aac_codec(InputPluginData* ip_data)
{
    return xstrdup("aac");
}

static const char *object_type_to_str(int object_type)
{
    switch (object_type) {
        case MAIN:	return "Main";
        case LC:	return "LC";
        case SSR:	return "SSR";
        case LTP:	return "LTP";
        case HE_AAC:	return "HE";
        case ER_LC:	return "ER-LD";
        case ER_LTP:	return "ER-LTP";
        case LD:	return "LD";
        case DRM_ER_LC:	return "DRM-ER-LC";
    }
    return NULL;
}

static char *aac_codec_profile(InputPluginData* ip_data)
{
    AACPrivate* priv = ip_data->private;
    const char *profile = object_type_to_str(priv->objectType);

    return profile ? xstrdup(profile) : NULL;
}

const static InputPluginOps ops = {
    .Open = aac_open,
    .Close = aac_close,
    .Read = aac_read,
    .Seek = aac_seek,
    .ReadComments = aac_read_comments,
    .Duration = aac_duration,
    .Bitrate = aac_bitrate,
    .BitrateCurrent = aac_current_bitrate,
    .Codec = aac_codec,
    .CodecProfile = aac_codec_profile
};

static const int priority = 50;
static const char* name = "aac";
static const char* const extensions[] = { "aac", NULL };
static const char* const mimeTypes[] = { "audio/aac", "audio/aacp", NULL };
static const InputPluginOpt options[] = { { NULL } };
static const unsigned abiVersion = INPUT_ABI_VERSION;

void aac_input_register(InputPlugin* plugin)
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
