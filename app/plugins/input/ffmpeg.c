//
// Created by dingjing on 2/13/23.
//
#include "ffmpeg.h"

#include "debug.h"
#include "utils.h"
#include "xmalloc.h"
#include "interface.h"
#include "../comment.h"

#include <stdio.h>
#include <libavutil/opt.h>
#include <libavformat/avio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#ifndef AVUTIL_MATHEMATICS_H
#include <libavutil/mathematics.h>
#endif

#ifndef AVCODEC_MAX_AUDIO_FRAME_SIZE
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000
#endif

struct _FfmpegInput
{
    AVPacket                pkt;
    int                     currPktSize;
    uint8_t*                currPktBuf;
    int                     streamIndex;

    unsigned long           currSize;
    unsigned long           currDuration;
};

struct _FfmpegOutput
{
    uint8_t*                buffer;
    uint8_t*                bufferMalloc;
    uint8_t*                bufferPos;	        /* current buffer position */
    int                     bufferUsedLen;
};

struct _FfmpegPrivate
{
    AVCodecContext*         codecContext;
    AVFormatContext*        inputContext;
    AVCodec*                codec;
    SwrContext*             swr;

    FfmpegInput*            input;
    FfmpegOutput*           output;
};

static FfmpegInput* ffmpeg_input_create(void)
{
    FfmpegInput *input = xnew(FfmpegInput, 1);

    if (av_new_packet(&input->pkt, 0) != 0) {
        free(input);
        return NULL;
    }
    input->currPktSize = 0;
    input->currPktBuf = input->pkt.data;
    return input;
}

static void ffmpeg_input_free(FfmpegInput *input)
{
#if LIBAVCODEC_VERSION_MAJOR >= 56
    av_packet_unref(&input->pkt);
#else
    av_free_packet(&input->pkt);
#endif
    free(input);
}

static FfmpegOutput *ffmpeg_output_create(void)
{
    FfmpegOutput *output = xnew(FfmpegOutput, 1);

    output->bufferMalloc = xnew(uint8_t, AVCODEC_MAX_AUDIO_FRAME_SIZE + 15);
    output->buffer = output->bufferMalloc;
    /* align to 16 bytes so avcodec can SSE/Altivec/etc */
    while ((intptr_t) output->buffer % 16)
        output->buffer += 1;
    output->bufferPos = output->buffer;
    output->bufferUsedLen = 0;
    return output;
}

static void ffmpeg_output_free(FfmpegOutput *output)
{
    free(output->bufferMalloc);
    output->bufferMalloc = NULL;
    output->buffer = NULL;
    free(output);
}

static inline void ffmpeg_buffer_flush(FfmpegOutput *output)
{
    output->bufferPos = output->buffer;
    output->bufferUsedLen = 0;
}

static void ffmpeg_init(void)
{
    static int inited = 0;

    if (inited != 0)
        return;
    inited = 1;

    av_log_set_level(AV_LOG_QUIET);

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 18, 100)
    /* We could register decoders explicitly to save memory, but we have to
	 * be careful about compatibility. */
	av_register_all();
#endif
}

static int ffmpeg_open(InputPluginData *ip_data)
{
    FfmpegPrivate *priv;
    int err = 0;
    int i;
    int stream_index = -1;
    int64_t channel_layout = 0;
    AVCodec *codec;
    AVCodecContext *cc = NULL;
    AVFormatContext *ic = NULL;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
    AVCodecParameters *cp = NULL;
#endif
    SwrContext *swr = NULL;

    ffmpeg_init();

    err = avformat_open_input(&ic, ip_data->fileName, NULL, NULL);
    if (err < 0) {
        DEBUG("av_open failed: %d\n", err);
        return -INPUT_ERROR_FILE_FORMAT;
    }

    do {
        err = avformat_find_stream_info(ic, NULL);
        if (err < 0) {
            DEBUG("unable to find stream info: %d\n", err);
            err = -INPUT_ERROR_FILE_FORMAT;
            break;
        }

        for (i = 0; i < ic->nb_streams; i++) {

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
            cp = ic->streams[i]->codecpar;
			if (cp->codec_type == AVMEDIA_TYPE_AUDIO) {
				stream_index = i;
				break;
			}
#else
            cc = ic->streams[i]->codec;
            if (cc->codec_type == AVMEDIA_TYPE_AUDIO) {
                stream_index = i;
                break;
            }
#endif
        }

        if (stream_index == -1) {
            DEBUG("could not find audio stream\n");
            err = -INPUT_ERROR_FILE_FORMAT;
            break;
        }

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
        codec = avcodec_find_decoder(cp->codec_id);
		cc = avcodec_alloc_context3(codec);
		avcodec_parameters_to_context(cc, cp);
#else
        codec = avcodec_find_decoder(cc->codec_id);
#endif
        if (!codec) {
            DEBUG("codec not found: %d, %s\n", cc->codec_id, avcodec_get_name(cc->codec_id));
            err = -INPUT_ERROR_UNSUPPORTED_FILE_TYPE;
            break;
        }

        if (codec->capabilities & AV_CODEC_CAP_TRUNCATED)
            cc->flags |= AV_CODEC_FLAG_TRUNCATED;

        if (avcodec_open2(cc, codec, NULL) < 0) {
            DEBUG("could not open codec: %d, %s\n", cc->codec_id, avcodec_get_name(cc->codec_id));
            err = -INPUT_ERROR_UNSUPPORTED_FILE_TYPE;
            break;
        }

        /* We assume below that no more errors follow. */
    } while (0);

    if (err < 0) {
        /* Clean up.  cc is never opened at this point.  (See above assumption.) */
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
        avcodec_free_context(&cc);
#endif
        avformat_close_input(&ic);
        return err;
    }

    priv = xnew(FfmpegPrivate, 1);
    priv->codecContext = cc;
    priv->inputContext = ic;
    priv->codec = codec;
    priv->input = ffmpeg_input_create();
    if (priv->input == NULL) {
        avcodec_close(cc);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
        avcodec_free_context(&cc);
#endif
        avformat_close_input(&ic);
        free(priv);
        return -INPUT_ERROR_INTERNAL;
    }
    priv->input->streamIndex = stream_index;
    priv->output = ffmpeg_output_create();

    /* Prepare for resampling. */
    swr = swr_alloc();
    av_opt_set_int(swr, "in_channel_layout",  av_get_default_channel_layout(cc->channels), 0);
    av_opt_set_int(swr, "out_channel_layout", av_get_default_channel_layout(cc->channels), 0);
    av_opt_set_int(swr, "in_sample_rate",     cc->sample_rate, 0);
    av_opt_set_int(swr, "out_sample_rate",    cc->sample_rate, 0);
    av_opt_set_sample_fmt(swr, "in_sample_fmt",  cc->sample_fmt, 0);
    priv->swr = swr;

    ip_data->private = priv;
    ip_data->sf = sf_rate(cc->sample_rate) | sf_channels(cc->channels);
    switch (cc->sample_fmt) {
        case AV_SAMPLE_FMT_U8:
            ip_data->sf |= sf_bits(8) | sf_signed(0);
            av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_U8,  0);
            break;
        case AV_SAMPLE_FMT_S32:
            ip_data->sf |= sf_bits(32) | sf_signed(1);
            av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S32,  0);
            break;
            /* AV_SAMPLE_FMT_S16 */
        default:
            ip_data->sf |= sf_bits(16) | sf_signed(1);
            av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16,  0);
            break;
    }
    swr_init(swr);
    ip_data->sf |= sf_host_endian();
    channel_layout = cc->channel_layout;
    channel_map_init_wave_ex(cc->channels, channel_layout, ip_data->channelMap);

    return 0;
}

static int ffmpeg_close(InputPluginData *ip_data)
{
    FfmpegPrivate *priv = ip_data->private;

    avcodec_close(priv->codecContext);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
    avcodec_free_context(&priv->codecContext);
#endif
    avformat_close_input(&priv->inputContext);
    swr_free(&priv->swr);
    ffmpeg_input_free(priv->input);
    ffmpeg_output_free(priv->output);
    free(priv);
    ip_data->private = NULL;
    return 0;
}

/*
 * This returns the number of bytes added to the buffer.
 * It returns < 0 on error.  0 on EOF.
 */
static int ffmpeg_fill_buffer(AVFormatContext* ic, AVCodecContext* cc, FfmpegInput* input, FfmpegOutput* output, SwrContext* swr)
{
#if LIBAVCODEC_VERSION_MAJOR >= 56
    AVFrame *frame = av_frame_alloc();
#else
    AVFrame *frame = avcodec_alloc_frame();
#endif
    int got_frame;
    while (1) {
        int len;

        if (input->currPktSize <= 0) {
#if LIBAVCODEC_VERSION_MAJOR >= 56
            av_packet_unref(&input->pkt);
#else
            av_free_packet(&input->pkt);
#endif
            if (av_read_frame(ic, &input->pkt) < 0) {
                /* Force EOF once we can read no longer. */
#if LIBAVCODEC_VERSION_MAJOR >= 56
                av_frame_free(&frame);
#else
                avcodec_free_frame(&frame);
#endif
                return 0;
            }
            if (input->pkt.stream_index == input->streamIndex) {
                input->currPktSize = input->pkt.size;
                input->currPktBuf = input->pkt.data;
                input->currSize += input->pkt.size;
                input->currDuration += input->pkt.duration;
            }
            continue;
        }

        {
            AVPacket avpkt;
            av_new_packet(&avpkt, input->currPktSize);
            memcpy(avpkt.data, input->currPktBuf, input->currPktSize);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
            int send_result = avcodec_send_packet(cc, &avpkt);
			if (send_result != 0) {
				if (send_result != AVERROR(EAGAIN)) {
					DEBUG("avcodec_send_packet() returned %d", send_result);
					char errstr[AV_ERROR_MAX_STRING_SIZE];
					if (!av_strerror(send_result, errstr, AV_ERROR_MAX_STRING_SIZE ))
					{
						DEBUG("av_strerror(): %s", errstr);
					} else {
						DEBUG("av_strerror(): Description for error cannot be found");
					}
					av_packet_unref(&avpkt);
					return -INPUT_ERROR_INTERNAL;
				}
				len = 0;
			} else {
				len = input->currPktSize;
			}

			int recv_result = avcodec_receive_frame(cc, frame);
			got_frame = (recv_result == 0) ? 1 : 0;
#else
            len = avcodec_decode_audio4(cc, frame, &got_frame, &avpkt);
#endif
#if LIBAVCODEC_VERSION_MAJOR >= 56
            av_packet_unref(&avpkt);
#else
            av_free_packet(&avpkt);
#endif
        }
        if (len < 0) {
            /* this is often reached when seeking, not sure why */
            input->currPktSize = 0;
            continue;
        }
        input->currPktSize -= len;
        input->currPktBuf += len;
        if (got_frame) {
            int res = swr_convert(swr,
                                  &output->buffer,
                                  frame->nb_samples,
                                  (const uint8_t **)frame->extended_data,
                                  frame->nb_samples);
            if (res < 0)
                res = 0;
            output->bufferPos = output->buffer;
            output->bufferUsedLen = res * cc->channels * sizeof(int16_t);
#if LIBAVCODEC_VERSION_MAJOR >= 56
            av_frame_free(&frame);
#else
            avcodec_free_frame(&frame);
#endif
            return output->bufferUsedLen;
        }
    }
    /* This should never get here. */
    return -INPUT_ERROR_INTERNAL;
}

static int ffmpeg_read(InputPluginData *ip_data, char *buffer, int count)
{
    FfmpegPrivate *priv = ip_data->private;
    FfmpegOutput *output = priv->output;
    int rc;
    int out_size;

    if (output->bufferUsedLen == 0) {
        rc = ffmpeg_fill_buffer(priv->inputContext, priv->codecContext, priv->input, priv->output, priv->swr);
        if (rc <= 0) {
            return rc;
        }
    }
    out_size = min_i(output->bufferUsedLen, count);
    memcpy(buffer, output->bufferPos, out_size);
    output->bufferUsedLen -= out_size;
    output->bufferPos += out_size;
    return out_size;
}

static int ffmpeg_seek(InputPluginData *ip_data, double offset)
{
    FfmpegPrivate *priv = ip_data->private;
    AVStream *st = priv->inputContext->streams[priv->input->streamIndex];
    int ret;

    int64_t pts = av_rescale_q(offset * AV_TIME_BASE, AV_TIME_BASE_Q, st->time_base);

    avcodec_flush_buffers(priv->codecContext);
    /* Force reading a new packet in next ffmpeg_fill_buffer(). */
    priv->input->currPktSize = 0;

    ret = av_seek_frame(priv->inputContext, priv->input->streamIndex, pts, 0);

    if (ret < 0) {
        return -INPUT_ERROR_FUNCTION_NOT_SUPPORTED;
    } else {
        ffmpeg_buffer_flush(priv->output);
        return 0;
    }
}

static void ffmpeg_read_metadata(struct growing_keyvals *c, AVDictionary *metadata)
{
    AVDictionaryEntry *tag = NULL;

    while ((tag = av_dict_get(metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        if (tag->value[0])
            comments_add_const(c, tag->key, tag->value);
    }
}

static int ffmpeg_read_comments (InputPluginData* ip_data, KeyValue** comments)
{
    FfmpegPrivate *priv = ip_data->private;
    AVFormatContext *ic = priv->inputContext;

    GROWING_KEY_VALUES(c);

    ffmpeg_read_metadata(&c, ic->metadata);
    for (unsigned i = 0; i < ic->nb_streams; i++) {
        ffmpeg_read_metadata(&c, ic->streams[i]->metadata);
    }

    key_value_terminate(&c);
    *comments = c.keyValues;

    return 0;
}

static int ffmpeg_duration(InputPluginData *ip_data)
{
    FfmpegPrivate *priv = ip_data->private;
    return priv->inputContext->duration / AV_TIME_BASE;
}

static long ffmpeg_bitrate(InputPluginData *ip_data)
{
    FfmpegPrivate *priv = ip_data->private;
    long bitrate = priv->inputContext->bit_rate;
    return bitrate ? bitrate : -INPUT_ERROR_FUNCTION_NOT_SUPPORTED;
}

static long ffmpeg_current_bitrate(InputPluginData *ip_data)
{
    FfmpegPrivate *priv = ip_data->private;
    AVStream *st = priv->inputContext->streams[priv->input->streamIndex];
    long bitrate = -1;
    /* ape codec returns silly numbers */
#if LIBAVCODEC_VERSION_MAJOR >= 55
    if (priv->codec->id == AV_CODEC_ID_APE)
#else
    if (priv->codec->id == CODEC_ID_APE)
#endif
        return -1;
    if (priv->input->currDuration > 0) {
        double seconds = priv->input->currDuration * av_q2d(st->time_base);
        bitrate = (8 * priv->input->currSize) / seconds;
        priv->input->currSize = 0;
        priv->input->currDuration = 0;
    }
    return bitrate;
}

static char *ffmpeg_codec(InputPluginData *ip_data)
{
    FfmpegPrivate *priv = ip_data->private;
    return xstrdup(priv->codec->name);
}

static char *ffmpeg_codec_profile(InputPluginData *ip_data)
{
    FfmpegPrivate *priv = ip_data->private;
    const char *profile;
    profile = av_get_profile_name(priv->codec, priv->codecContext->profile);
    return profile ? xstrdup(profile) : NULL;
}

static const InputPluginOps ops = {
    .Open = ffmpeg_open,
    .Close = ffmpeg_close,
    .Read = ffmpeg_read,
    .Seek = ffmpeg_seek,
    .ReadComments = ffmpeg_read_comments,
    .Duration = ffmpeg_duration,
    .Bitrate = ffmpeg_bitrate,
    .BitrateCurrent = ffmpeg_current_bitrate,
    .Codec = ffmpeg_codec,
    .CodecProfile = ffmpeg_codec_profile
};

static const int priority = 30;
static const char* name = "ffmpeg";
static const char *const extensions[] = {
    "aa", "aac", "ac3", "aif", "aifc", "aiff", "ape", "au", "fla", "flac",
    "m4a", "m4b", "mka", "mkv", "mp+", "mp2", "mp3", "mp4", "mpc", "mpp",
    "ogg", "opus", "shn", "tak", "tta", "wav", "webm", "wma", "wv",
    NULL
};
static const char *const mimeTypes[] = { NULL };
static const InputPluginOpt options[] = { { NULL } };
static const unsigned abiVersion = INPUT_ABI_VERSION;

void ffmpeg_input_register(InputPlugin *plugin)
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
