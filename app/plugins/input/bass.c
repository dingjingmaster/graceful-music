//
// Created by dingjing on 2/13/23.
//

#include "bass.h"

#include "interface.h"

#include "xmalloc.h"
#include "../uchar.h"
#include "../../comment.h"

#define CHANS                       (2)
#define BITS                        (16)
#define FREQ                        (44100)

typedef struct _BassPrivate         BassPrivate;

struct _BassPrivate
{
    unsigned long                   chan;
};

static int bass_init (void)
{
    static int inited = 0;

    if (inited)
        return 1;

    if (!BASS_Init(0, FREQ, 0, 0, NULL))
        return 0;

    inited = 1;
    return 1;
}

static int bass_open (InputPluginData* ip_data)
{
    BassPrivate* priv;
    unsigned long chan;
    unsigned long flags;

    if (!bass_init())
        return -INPUT_ERROR_INTERNAL;

    flags = BASS_MUSIC_DECODE;
    flags |= BASS_MUSIC_RAMP;
    flags |= BASS_MUSIC_PRESCAN;
    flags |= BASS_MUSIC_STOPBACK;

    chan = BASS_MusicLoad(FALSE, ip_data->fileName, 0, 0, flags, 0);

    if (!chan) {
        return -INPUT_ERROR_ERRNO;
    }

    priv = xnew (BassPrivate, 1);
    priv->chan = chan;
    ip_data->private = priv;
    ip_data->sf = sf_bits(BITS) | sf_rate(FREQ) | sf_channels(CHANS) | sf_signed(1);
    ip_data->sf |= sf_host_endian();
    channel_map_init_stereo(ip_data->channelMap);
    return 0;
}

static int bass_close(InputPluginData* ip_data)
{
     BassPrivate *priv = ip_data->private;

    BASS_MusicFree(priv->chan);
    free(priv);
    ip_data->private = NULL;
    return 0;
}

static int bass_read(InputPluginData* ip_data, char *buffer, int count)
{
    int length;
     BassPrivate *priv = ip_data->private;
    length = BASS_ChannelGetData(priv->chan, buffer, count);
    if (length < 0) {
        return 0;
    }
    return length;
}

static int bass_seek(InputPluginData* ip_data, double offset)
{
     BassPrivate *priv = ip_data->private;
    QWORD pos = (QWORD)(offset * (FREQ * CHANS * (BITS / 8)) + 0.5);
    QWORD flags = BASS_POS_BYTE | BASS_POS_DECODE;

    if (!BASS_ChannelSetPosition(priv->chan, pos, flags)) {
        return -INPUT_ERROR_INTERNAL;
    }
    return 0;
}

static unsigned char *encode_ascii_string(const char *str)
{
    unsigned char *ret;
    int n;

    ret = malloc(strlen(str) + 1);
    n = u_to_ascii(ret, str, strlen(str));
    ret[n] = '\0';
    return ret;
}

static int bass_read_comments(InputPluginData* ip_data, KeyValue** comments)
{
     BassPrivate *priv = ip_data->private;
    GROWING_KEY_VALUES(c);
    const char *val;

    val = BASS_ChannelGetTags(priv->chan, BASS_TAG_MUSIC_NAME);
    if (val && val[0])
        comments_add_const(&c, "title", encode_ascii_string(val));
    key_value_terminate(&c);
    *comments = c.keyValues;

    return 0;
}

static int bass_duration(InputPluginData* ip_data)
{
    static float length = 0;
    int pos;
     BassPrivate *priv = ip_data->private;

    pos = BASS_ChannelGetLength(priv->chan, BASS_POS_BYTE);
    if (pos && pos != -1) {
        length = BASS_ChannelBytes2Seconds(priv->chan, pos);
    }
    else {
        length = -INPUT_ERROR_FUNCTION_NOT_SUPPORTED;
    }
    return length;
}

static long bass_bitrate(InputPluginData* ip_data)
{
    return -INPUT_ERROR_FUNCTION_NOT_SUPPORTED;
}

static const char *bass_type_to_string(int type)
{
    /* from <bass.h> */
    switch (type) {
        case 0x20000: return "mod";
        case 0x20001: return "mtm";
        case 0x20002: return "s3m";
        case 0x20003: return "xm";
        case 0x20004: return "it";
    }
    return NULL;
}

static char *bass_codec(InputPluginData* ip_data)
{
    const char *codec;
    int type;
    BASS_CHANNELINFO info;
     BassPrivate *priv = ip_data->private;

    if (!(BASS_ChannelGetInfo(priv->chan, &info))) {
        return NULL;
    }
    type = info.ctype;
    codec = bass_type_to_string(type);
    return codec ? xstrdup(codec) : NULL;
}

static char *bass_codec_profile(InputPluginData* ip_data)
{
    return NULL;
}

const static InputPluginOps ops = {
    .Open = bass_open,
    .Close = bass_close,
    .Read = bass_read,
    .Seek = bass_seek,
    .ReadComments = bass_read_comments,
    .Duration = bass_duration,
    .Bitrate = bass_bitrate,
    .BitrateCurrent = bass_bitrate,
    .Codec = bass_codec,
    .CodecProfile = bass_codec_profile
};

static const int priority = 60;
static const char* name = "bass";
static const char * const extensions[] = {
    "xm", "it", "s3m", "mod", "mtm", "umx", NULL
};

static const char* const mimeTypes[] = { NULL };
static const InputPluginOpt options[] = { { NULL } };
static const unsigned abiVersion = INPUT_ABI_VERSION;

void bass_input_register(InputPlugin *plugin)
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
