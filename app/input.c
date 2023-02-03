//
// Created by dingjing on 2/2/23.
//

#include "input.h"

#include <glib.h>
#include <fcntl.h>
#include <unistd.h>

#include "log.h"
#include "utils.h"

#define INPUT_READ_LOCK()               pthread_rwlock_rdlock(&gInputLock)
#define INPUT_WRITE_LOCK()              pthread_rwlock_wrlock(&gInputLock)
#define INPUT_UNLOCK()                  pthread_rwlock_unlock(&gInputLock)

#include "input-interface.h"

typedef struct _Input                   Input;

struct _InputPlugin
{
    const InputPluginOps*               ops;
    InputPluginData                     data;

    unsigned int                        open : 1;
    unsigned int                        eof : 1;

    int                                 httpCode;
    char*                               httpReason;

    int                                 duration;       // -1 表示未设置
    int                                 bitrate;        //
    char*                               codec;
    char*                               codecProfile;

    // pcm is converted to 16-bit signed little-endian stereo
    // Note: no conversion is done if channels > 2 or bits > 16
    void    (*PCMConvert)               (void*, const void*, int);
    void    (*PCMConvertInPlace)        (void*, int);

    /**
     * 4    if 8-bit mono
     * 2    if 8-bit stereo or 16-bit mono
     * 1    otherwise
     */
     int                                pcmConvertScale;
};

struct _Input
{
    GList                               node;

    char*                               name;
    void*                               handle;

    int                                 priority;
    const char* const*                  extensions;
    const char* const*                  mimeTypes;

    const InputPluginOps*               ops;
    const InputPluginOpt*               options;
};

static const char*                      gPluginDir;
static GList                            gInputHead;
static pthread_rwlock_t                 gInputLock = PTHREAD_RWLOCK_INITIALIZER;
static int                              gHttpConnectionTimeout = 5e3;
static int                              gHttpReadTimeout = 5e3;
static const char*                      gMimeTypes[] = {
    "audio/m3u",
    "audio/x-scpls",
    "audio/x-mpegurl"
};

static int open_file (InputPlugin* ip);
static const InputPluginOps* get_ops_by_extension (const char* ext, GList** head);

static void input_reset     (InputPlugin* ip, int closeFd);
static void input_init      (InputPlugin* ip, const char* fileName);

void input_load_plugins (void)
{

}

InputPlugin* input_new (const char* fileName)
{
    InputPlugin* ip = (InputPlugin*) malloc (sizeof(InputPlugin));

    input_init (ip, fileName);

    return ip;
}

int input_close (InputPlugin* ip)
{
    int rc = ip->ops->Close(&ip->data);

    if (-1 != ip->data.fd) {
        close (ip->data.fd);
    }

    free (ip->httpReason);
    free (ip->data.icyURL);
    free (ip->data.icyName);
    free (ip->data.metaData);
    free (ip->data.icyGenre);

    input_init (ip, ip->data.fileName);

    return rc;
}

void input_delete (InputPlugin* ip)
{
    if (ip->open) {
        input_close (ip);
    }

    free (ip->data.fileName);
    free (ip);
}

int input_open (InputPlugin* ip)
{
    int rc = 0;

    if (ip->data.remote) {
        // FIXME:// DJ-
    }
    else {
        // cdda url 打开
        // cue url 打开

        rc = open_file (ip);
    }

    if (rc) {
        WARNING ("opening '%s' failed: %d %s\n", ip->data.fileName, rc, rc == -1 ? strerror (errno) : "");
        input_reset (ip, 1);

        return rc;
    }

    ip->open = 1;

    return 0;
}






static void input_init (InputPlugin* ip, const char* fileName)
{
    const InputPlugin t = {
        .httpCode               = -1,
        .pcmConvertScale        = -1,
        .duration               = -1,
        .bitrate                = -1,
        .data = {
            .fd             = -1,
            .fileName       = g_strdup (fileName),
            .remote         = (g_str_has_prefix (fileName, "http://") || g_str_has_prefix (fileName, "https://")),
            .channelMap     = CHANNEL_MAP_INIT
        }
    };

    *ip = t;
}

static void input_reset (InputPlugin* ip, int closeFd)
{
    int fd = ip->data.fd;

    free (ip->data.metaData);
    input_init (ip, ip->data.fileName);

    if (-1 != fd) {
        if (closeFd) {
            close (fd);
        }
        else {
            lseek (fd, 0, SEEK_SET);
            ip->data.fd = fd;
        }
    }
}

static int open_file (InputPlugin* ip)
{
    int rv = 0;

    INPUT_READ_LOCK();

    GList* head = &gInputHead;

    const char* ext = path_get_extension (ip->data.fileName);
    if (!ext) {
        rv = -INPUT_ERROR_UNRECOGNIZED_FILE_TYPE;
        goto end;
    }

    InputPluginOps* ops = get_ops_by_extension (ext, &head);
    if (!ops) {
        rv = -INPUT_ERROR_UNRECOGNIZED_FILE_TYPE;
        goto end;
    }

    ip->data.fd = open (ip->data.fileName, O_RDONLY);
    if (-1 == ip->data.fd) {
        rv = -INPUT_ERROR_ERRNO;
        goto end;
    }

    while (1) {
        ip->ops = ops;
        rv = ip->ops->Open (&ip->data);
        if (INPUT_ERROR_UNSUPPORTED_FILE_TYPE != rv) {
            break;
        }

        ops = get_ops_by_extension (ext, &head);
        if (!ops) {
            break;
        }

        input_reset (ip, 0);
        DEBUG("fallback: try next plugin for '%s'", ip->data.fileName);
    }

end:
    INPUT_UNLOCK();

    return rv;
}

static const InputPluginOps* get_ops_by_extension (const char* ext, GList** head)
{
    InputPluginOps* rv = NULL;

    INPUT_READ_LOCK();

    for (GList* node = *head; node != NULL; node = node->next) {
        Input* ip = (Input*) node->data;
        const char* const* exts = ip->extensions;
        int i = 0;
        if (0 >= ip->priority) {
            break;
        }
        for (i = 0; exts[i]; ++i) {
            if (0 == strcasecmp (ext, exts[i]) || 0 == strcmp ("*", exts[i])) {
                *head = node;
                rv = ip->ops;
                goto end;
            }
        }
    }

end:
    INPUT_UNLOCK();

    return rv;
}
