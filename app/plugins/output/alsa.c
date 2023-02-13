//
// Created by dingjing on 2/10/23.
//

#include "alsa.h"

/*
 * snd_pcm_state_t:
 *
 * Open
 * SND_PCM_STATE_OPEN = 0,
 *
 * Setup installed
 * SND_PCM_STATE_SETUP = 1,
 *
 * Ready to start
 * SND_PCM_STATE_PREPARED = 2,
 *
 * Running
 * SND_PCM_STATE_RUNNING = 3,
 *
 * Stopped: underrun (playback) or overrun (capture) detected
 * SND_PCM_STATE_XRUN = 4,
 *
 * Draining: running (playback) or stopped (capture)
 * SND_PCM_STATE_DRAINING = 5,
 *
 * Paused
 * SND_PCM_STATE_PAUSED = 6,
 *
 * Hardware is suspended
 * SND_PCM_STATE_SUSPENDED = 7,
 *
 * Hardware is disconnected
 * SND_PCM_STATE_DISCONNECTED = 8,
 */
#include "log.h"
#include "../utils.h"
#include "../interface.h"
#include "../common/sf.h"
#include "../common/xmalloc.h"

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

#include <alsa/asoundlib.h>

static SampleFormat             alsaSf;
static snd_pcm_t*               alsaHandle;
static snd_pcm_format_t         alsaFmt;
static int                      alsaCanPause;
static snd_pcm_status_t*        status;

/* bytes (bits * channels / 8) */
static int                      alsaFrameSize;

/* configuration */
static char*                    alsaDspDevice = "default";


static int alsa_error_to_op_error (int err)
{
    if (!err) {
        return OUTPUT_ERROR_SUCCESS;
    }
    err = -err;
    if (err < SND_ERROR_BEGIN) {
        errno = err;
        return -OUTPUT_ERROR_ERRNO;
    }

    return -OUTPUT_ERROR_INTERNAL;
}

/* we don't want error messages to stderr */
static void error_handler (const char *file, int line, const char *function, int err, const char *fmt, ...)
{
}

static int op_alsa_init (void)
{
    int rc;

    snd_lib_error_set_handler(error_handler);

    if (alsaDspDevice == NULL) {
        alsaDspDevice = xstrdup("default");
    }

    rc = snd_pcm_status_malloc(&status);
    if (rc < 0) {
        free(alsaDspDevice);
        alsaDspDevice = NULL;
        errno = ENOMEM;
        return -OUTPUT_ERROR_ERRNO;
    }

    return OUTPUT_ERROR_SUCCESS;
}

static int op_alsa_exit (void)
{
    snd_pcm_status_free (status);
    free (alsaDspDevice);
    alsaDspDevice = NULL;

    return OUTPUT_ERROR_SUCCESS;
}

/* randomize hw params */
static int alsa_set_hw_params (void)
{
    snd_pcm_hw_params_t *hwparams = NULL;
    unsigned int buffer_time_max = 300 * 1000; /* us */
    const char *cmd;
    unsigned int rate;
    int rc, dir;

    snd_pcm_hw_params_malloc(&hwparams);

    cmd = "snd_pcm_hw_params_any";
    rc = snd_pcm_hw_params_any(alsaHandle, hwparams);
    if (rc < 0) {
        goto error;
    }

    cmd = "snd_pcm_hw_params_set_buffer_time_max";
    rc = snd_pcm_hw_params_set_buffer_time_max(alsaHandle, hwparams, &buffer_time_max, &dir);
    if (rc < 0) {
        goto error;
    }

    alsaCanPause = snd_pcm_hw_params_can_pause(hwparams);
    DEBUG ("can pause = %d", alsaCanPause);

    cmd = "snd_pcm_hw_params_set_access";
    rc = snd_pcm_hw_params_set_access(alsaHandle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (rc < 0) {
        goto error;
    }

    alsaFmt = snd_pcm_build_linear_format(sf_get_bits(alsaSf), sf_get_bits(alsaSf), sf_get_signed(alsaSf) ? 0 : 1, sf_get_bigendian(alsaSf));
    cmd = "snd_pcm_hw_params_set_format";
    rc = snd_pcm_hw_params_set_format(alsaHandle, hwparams, alsaFmt);
    if (rc < 0) {
        goto error;
    }

    cmd = "snd_pcm_hw_params_set_channels";
    rc = snd_pcm_hw_params_set_channels(alsaHandle, hwparams, sf_get_channels(alsaSf));
    if (rc < 0) {
        goto error;
    }

    cmd = "snd_pcm_hw_params_set_rate";
    rate = sf_get_rate(alsaSf);
    dir = 0;
    rc = snd_pcm_hw_params_set_rate_near(alsaHandle, hwparams, &rate, &dir);
    if (rc < 0) {
        goto error;
    }
    DEBUG ("rate=%d", rate);

    cmd = "snd_pcm_hw_params";
    rc = snd_pcm_hw_params(alsaHandle, hwparams);
    if (rc < 0) {
        goto error;
    }
    goto out;

error:
    DEBUG ("%s: error: %s", cmd, snd_strerror(rc));

out:
    snd_pcm_hw_params_free(hwparams);

    return rc;
}

static int op_alsa_open(SampleFormat sf, const ChannelPosition* channelMap)
{
    alsaSf = sf;
    alsaFrameSize = sf_get_frame_size(alsaSf);

    int rc = snd_pcm_open(&alsaHandle, alsaDspDevice, SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        ERROR ("snd_pcm_open error, exit, error: '%s'", g_strerror (errno));
        goto error;
    }

    rc = alsa_set_hw_params();
    if (rc) {
        goto close_error;
    }

    rc = snd_pcm_prepare(alsaHandle);
    if (rc < 0) {
        goto close_error;
    }

    return OUTPUT_ERROR_SUCCESS;

close_error:
    snd_pcm_close(alsaHandle);

error:
    return alsa_error_to_op_error(rc);
}

static int op_alsa_write(const char *buffer, int count);

static int op_alsa_close(void)
{
    int rc;

    rc = snd_pcm_drain(alsaHandle);
    DEBUG ("snd_pcm_drain, rc: %d", rc);

    rc = snd_pcm_close(alsaHandle);
    DEBUG ("snd_pcm_close, rc: %d", rc);

    return alsa_error_to_op_error(rc);
}

static int op_alsa_drop(void)
{
    int rc;

    rc = snd_pcm_drop(alsaHandle);
    DEBUG ("snd_pcm_drop, rc: %d", rc);

    rc = snd_pcm_prepare(alsaHandle);
    DEBUG ("snd_pcm_prepare, rc: %d", rc);

    /* drop set state to SETUP
     * prepare set state to PREPARED
     *
     * so if old state was PAUSED we can't UNPAUSE (see op_alsa_unpause)
     */
    return alsa_error_to_op_error(rc);
}

static int op_alsa_write(const char *buffer, int count)
{
    int rc, len;
    int recovered = 0;

    len = count / alsaFrameSize;
again:
    rc = snd_pcm_writei(alsaHandle, buffer, len);
    if (rc < 0) {
        // rc _should_ be either -EBADFD, -EPIPE or -ESTRPIPE
        if (!recovered && (rc == -EINTR || rc == -EPIPE || rc == -ESTRPIPE)) {
            DEBUG ("snd_pcm_writei failed: %s, trying to recover", snd_strerror(rc));
            recovered++;
            // this handles -EINTR, -EPIPE and -ESTRPIPE
            // for other errors it just returns the error code
            rc = snd_pcm_recover(alsaHandle, rc, 1);
            if (!rc) {
                goto again;
            }
        }

        /* this handles EAGAIN too which is not critical error */
        return alsa_error_to_op_error(rc);
    }

    rc *= alsaFrameSize;

    return rc;
}

static int op_alsa_buffer_space(void)
{
    int rc;
    snd_pcm_sframes_t f;

    f = snd_pcm_avail_update(alsaHandle);
    while (f < 0) {
        DEBUG ("snd_pcm_avail_update failed: %s, trying to recover", snd_strerror(f));
        rc = snd_pcm_recover(alsaHandle, f, 1);
        if (rc < 0) {
            DEBUG ("recovery failed: %s", snd_strerror(rc));
            return alsa_error_to_op_error(rc);
        }
        f = snd_pcm_avail_update(alsaHandle);
    }

    return f * alsaFrameSize;
}

static int op_alsa_pause(void)
{
    int rc = 0;
    if (alsaCanPause) {
        snd_pcm_state_t state = snd_pcm_state(alsaHandle);
        if (state == SND_PCM_STATE_PREPARED) {
            // state is PREPARED -> no need to pause
        } else if (state == SND_PCM_STATE_RUNNING) {
            // state is RUNNING - > pause

            // infinite timeout
            rc = snd_pcm_wait(alsaHandle, -1);
            DEBUG ("snd_pcm_wait, rc: %d", rc);

            rc = snd_pcm_pause(alsaHandle, 1);
            DEBUG ("snd_pcm_pause, rc: %d", rc);
        } else {
            DEBUG ("error: state is not RUNNING or PREPARED");
            rc = -OUTPUT_ERROR_INTERNAL;
        }
    } else {
        rc = snd_pcm_drop(alsaHandle);
        DEBUG ("snd_pcm_drop, rc: %d", rc);
    }
    return alsa_error_to_op_error(rc);
}

static int op_alsa_unpause(void)
{
    int rc = 0;
    if (alsaCanPause) {
        snd_pcm_state_t state = snd_pcm_state(alsaHandle);
        if (state == SND_PCM_STATE_PREPARED) {
            // state is PREPARED -> no need to unpause
        } else if (state == SND_PCM_STATE_PAUSED) {
            // state is PAUSED -> unpause

            // infinite timeout
            rc = snd_pcm_wait(alsaHandle, -1);
            DEBUG ("snd_pcm_wait, rc: %d", rc);

            rc = snd_pcm_pause(alsaHandle, 0);
            DEBUG ("snd_pcm_pause, rc: %d", rc);
        } else {
            DEBUG ("error: state is not PAUSED nor PREPARED");
            rc = -OUTPUT_ERROR_INTERNAL;
        }
    } else {
        rc = snd_pcm_prepare(alsaHandle);
        DEBUG ("snd_pcm_prepare, rc: %d", rc);
    }
    return alsa_error_to_op_error(rc);
}

static int op_alsa_set_device(const char *val)
{
    free(alsaDspDevice);
    alsaDspDevice = xstrdup(val);

    return OUTPUT_ERROR_SUCCESS;
}

static int op_alsa_get_device (char** val)
{
    if (alsaDspDevice)
        *val = xstrdup(alsaDspDevice);
    return OUTPUT_ERROR_SUCCESS;
}

static const OutputPluginOps ops = {
    .Init = op_alsa_init,
    .Exit = op_alsa_exit,
    .Open = op_alsa_open,
    .Close = op_alsa_close,
    .Drop = op_alsa_drop,
    .Write = op_alsa_write,
    .BufferSpace = op_alsa_buffer_space,
    .Pause = op_alsa_pause,
    .Unpause = op_alsa_unpause,
};

static const char* name = "alsa";

static const OutputPluginOpt options[] = {
    OPT(op_alsa, device),
    { NULL },
};

static const int priority = 0;
static const unsigned int abiVersion = OUTPUT_ABI_VERSION;

extern MixerPluginOps mixerOps;
extern MixerPluginOpt mixerOpts;

void alsa_output_register (OutputPlugin* plugin)
{
    plugin->name = name;
    plugin->pcmOps = &ops;
    plugin->abiVersion = abiVersion;
    plugin->pcmOptions = &options;
    plugin->pcmInitialized = 0;
    plugin->mixerInitialized = 0;

    plugin->mixerOps = &mixerOps;
    plugin->mixerOptions = &mixerOpts;
}
