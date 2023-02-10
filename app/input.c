//
// Created by dingjing on 2/10/23.
//

#include "input.h"

#include "log.h"
#include "pcm.h"
#include "http.h"
#include "file.h"
#include "cmus.h"
#include "path.h"
#include "list.h"
#include "misc.h"
#include "debug.h"
#include "utils.h"
#include "options.h"
#include "locking.h"
#include "xmalloc.h"
#include "convert.h"
#include "xstrjoin.h"
#include "ui_curses.h"
#include "mergesort.h"
#include "input-interface.h"

#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <strings.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>


typedef struct _Input                   Input;


// FIXME:// DJ- 后续要修改
extern const char* libDir;
extern const char* dataDir;

struct _InputPlugin
{
    const InputPluginOps*               ops;
    InputPluginData                     data;
    unsigned int                        open : 1;
    unsigned int                        eof : 1;
    int                                 httpCode;
    char*                               httpReason;

    int                                 duration;       // cached duration, -1 = unset
    long                                bitrate;        // cached bitrate, -1 = unset
    char*                               codec;          // cached codec, NULL = unset
    char*                               codecProfile;   // cached codec_profile, NULL = unset

    /*
     * pcm is converted to 16-bit signed little-endian stereo
     * NOTE: no conversion is done if channels > 2 or bits > 16
     */
    void (*PcmConvert)          (void*, const void*, int);
    void (*PcmConvertInPlace)   (void*, int);
    /*
     * 4  if 8-bit mono
     * 2  if 8-bit stereo or 16-bit mono
     * 1  otherwise
     */
    int                                 pcmConvertScale;
};

struct _Input
{
    GList*                              node;
    char*                               name;
    void*                               handle;

    int                                 priority;
    const char* const*                  extensions;
    const char* const*                  mimeTypes;
    const InputPluginOps*               ops;
    const InputPluginOpt*               options;
};

static const char* pluginDir;
//static LIST_HEAD(ip_head);
//static GSList*                          gInputHead;
//gInputPlugins;

/* protects ip->priority and ip_head */
static pthread_rwlock_t ip_lock = CMUS_RWLOCK_INITIALIZER;

#define ip_rdlock() cmus_rwlock_rdlock(&ip_lock)
#define ip_wrlock() cmus_rwlock_wrlock(&ip_lock)
#define ip_unlock() cmus_rwlock_unlock(&ip_lock)

/* timeouts (ms) */
static int http_connection_timeout = 5e3;
static int http_read_timeout = 5e3;

static const char *pl_mime_types[] = {
    "audio/m3u",
    "audio/x-scpls",
    "audio/x-mpegurl"
};

/**
 * @brief
 *  通过文件扩展名获取 plugin
 */
static const InputPluginOps* get_ops_by_extension_locked (const char* ext)
//    static const InputPluginOps* get_ops_by_extension_locked (const char* ext, struct list_head** pHeader)
{
    GHashTable* index = gInputPlugins->pluginIndex;

    if (g_hash_table_contains (index, ext)) {
        InputPlugin* p = (InputPlugin*) g_hash_table_lookup (index, ext);
        return p->ops;
    }

//    struct list_head *node = *headp;
//
//    for (node = node->next; node != &ip_head; node = node->next) {
//        Input* ip = list_entry (node, Input, node);
//        const char * const *exts = ip->extensions;
//        int i;
//
//        if (ip->priority <= 0) {
//            break;
//        }
//
//        for (i = 0; exts[i]; i++) {
//            if (strcasecmp(ext, exts[i]) == 0 || strcmp("*", exts[i]) == 0) {
//                *headp = node;
//                return ip->ops;
//            }
//        }
//    }
//
//    for (GSList* node = *pHeader; node != NULL;)
//

    return NULL;
}

static const InputPluginOps* get_ops_by_extension (const char *ext)
{
    ip_rdlock();
    const InputPluginOps* rv = get_ops_by_extension_locked (ext);
    ip_unlock();
    return rv;
}

static const InputPluginOps* get_ops_by_mime_type_locked (const char *mime_type)
{
    Input* ip;

//    list_for_each_entry(ip, &ip_head, node) {
//        const char * const *types = ip->mimeTypes;
//        int i;
//
//        if (ip->priority <= 0) {
//            break;
//        }
//
//        for (i = 0; types[i]; i++) {
//            if (strcasecmp(mime_type, types[i]) == 0)
//                return ip->ops;
//        }
//    }
    return NULL;
}

static const InputPluginOps* get_ops_by_mime_type (const char* mimeType)
{
    ip_rdlock();
    const InputPluginOps* rv = get_ops_by_mime_type_locked (mimeType);
    ip_unlock();

    return rv;
}

static void key_value_add_basic_auth(struct growing_keyvals* c,
                                   const char* user,
                                   const char* pass,
                                   const char* header)
{
    char buf[256];
    char *encoded;

    snprintf(buf, sizeof(buf), "%s:%s", user, pass);
    encoded = base64_encode(buf);
    if (encoded == NULL) {
        d_print("couldn't base64 encode '%s'\n", buf);
    } else {
        snprintf(buf, sizeof(buf), "Basic %s", encoded);
        free(encoded);
        key_value_add (c, header, xstrdup(buf));
    }
}

static int do_http_get(struct http_get *hg, const char *uri, int redirections)
{
    GROWING_KEY_VALUES(h);
    int i, rc;
    const char *val;
    char *redirloc;

    d_print("%s\n", uri);

    hg->headers = NULL;
    hg->reason = NULL;
    hg->proxy = NULL;
    hg->code = -1;
    hg->fd = -1;
    if (http_parse_uri(uri, &hg->uri))
        return -INPUT_ERROR_INVALID_URI;

    if (http_open(hg, http_connection_timeout))
        return -INPUT_ERROR_ERRNO;

    key_value_add(&h, "Host", xstrdup(hg->uri.host));
    if (hg->proxy && hg->proxy->user && hg->proxy->pass)
        key_value_add_basic_auth(&h, hg->proxy->user, hg->proxy->pass, "Proxy-Authorization");
    key_value_add(&h, "User-Agent", xstrdup("cmus/" VERSION));
    key_value_add(&h, "Icy-MetaData", xstrdup("1"));
    if (hg->uri.user && hg->uri.pass)
        key_value_add_basic_auth(&h, hg->uri.user, hg->uri.pass, "Authorization");
    key_value_terminate(&h);

    rc = http_get(hg, h.keyValues, http_read_timeout);
    key_value_free(h.keyValues);
    switch (rc) {
        case -1:
            return -INPUT_ERROR_ERRNO;
        case -2:
            return -INPUT_ERROR_HTTP_RESPONSE;
    }

    DEBUG ("HTTP response: %d %s\n", hg->code, hg->reason);
    for (i = 0; hg->headers[i].key != NULL; i++)
        DEBUG ("  %s: %s\n", hg->headers[i].key, hg->headers[i].value);

    switch (hg->code) {
        case 200: /* OK */
            return 0;
            /*
             * 3xx Codes (Redirections)
             *     unhandled: 300 Multiple Choices
             */
        case 301: /* Moved Permanently */
        case 302: /* Found */
        case 303: /* See Other */
        case 307: /* Temporary Redirect */
            val = key_value_get_value(hg->headers, "location");
            if (!val)
                return -INPUT_ERROR_HTTP_RESPONSE;

            redirections++;
            if (redirections > 2)
                return -INPUT_ERROR_HTTP_REDIRECT_LIMIT;

            redirloc = xstrdup(val);
            http_get_free(hg);
            close(hg->fd);

            rc = do_http_get(hg, redirloc, redirections);

            free(redirloc);
            return rc;
        default:
            return -INPUT_ERROR_HTTP_STATUS;
    }
}

static int setup_remote(InputPlugin* ip, const struct keyval *headers, int sock)
{
    const char *val;

    val = key_value_get_value (headers, "Content-Type");
    if (val) {
        d_print("Content-Type: %s\n", val);
        ip->ops = get_ops_by_mime_type(val);
        if (ip->ops == NULL) {
            d_print("unsupported content type: %s\n", val);
            close(sock);
            return -INPUT_ERROR_FILE_FORMAT;
        }
    } else {
        const char *type = "audio/mpeg";

        d_print("assuming %s content type\n", type);
        ip->ops = get_ops_by_mime_type(type);
        if (ip->ops == NULL) {
            DEBUG ("unsupported content type: %s", type);
            close(sock);
            return -INPUT_ERROR_FILE_FORMAT;
        }
    }

    ip->data.fd = sock;
    ip->data.metaData = xnew(char, 16 * 255 + 1);

    val = key_value_get_value (headers, "icy-metaint");
    if (val) {
        long int lint;

        if (str_to_int(val, &lint) == 0 && lint >= 0) {
            ip->data.metaInt = lint;
            DEBUG ("metaint: %d", ip->data.metaInt);
        }
    }

    val = key_value_get_value (headers, "icy-name");
    if (val)
        ip->data.icyName = to_utf8(val, icecast_default_charset);

    val = key_value_get_value (headers, "icy-genre");
    if (val)
        ip->data.icyGenre = to_utf8(val, icecast_default_charset);

    val = key_value_get_value (headers, "icy-url");
    if (val)
        ip->data.icyURL = to_utf8(val, icecast_default_charset);

    return 0;
}

struct read_playlist_data {
    InputPlugin* ip;
    int rc;
    int count;
};

static int handle_line(void *data, const char *uri)
{
    struct read_playlist_data *rpd = data;
    struct http_get hg;

    rpd->count++;
    rpd->rc = do_http_get(&hg, uri, 0);
    if (rpd->rc) {
        rpd->ip->httpCode = hg.code;
        rpd->ip->httpReason = hg.reason;
        if (hg.fd >= 0)
            close(hg.fd);

        hg.reason = NULL;
        http_get_free(&hg);
        return 0;
    }

    rpd->rc = setup_remote(rpd->ip, hg.headers, hg.fd);
    http_get_free(&hg);
    return 1;
}

static int read_playlist(InputPlugin* ip, int sock)
{
    struct read_playlist_data rpd = { ip, 0, 0 };
    char *body;
    size_t size;

    body = http_read_body(sock, &size, http_read_timeout);
    close(sock);
    if (!body)
        return -INPUT_ERROR_ERRNO;

    cmus_playlist_for_each(body, size, 0, handle_line, &rpd);
    free(body);
    if (!rpd.count) {
        d_print("empty playlist\n");
        rpd.rc = -INPUT_ERROR_HTTP_RESPONSE;
    }
    return rpd.rc;
}

static int open_remote(InputPlugin* ip)
{
    InputPluginData* d = &ip->data;
    struct http_get hg;
    const char *val;
    int rc;

    rc = do_http_get(&hg, d->fileName, 0);
    if (rc) {
        ip->httpCode = hg.code;
        ip->httpReason = hg.reason;
        hg.reason = NULL;
        http_get_free(&hg);
        return rc;
    }

    val = key_value_get_value (hg.headers, "Content-Type");
    if (val) {
        int i;

        for (i = 0; i < N_ELEMENTS(pl_mime_types); i++) {
            if (!strcasecmp(val, pl_mime_types[i])) {
                d_print("Content-Type: %s\n", val);
                http_get_free(&hg);
                return read_playlist(ip, hg.fd);
            }
        }
    }

    rc = setup_remote(ip, hg.headers, hg.fd);
    http_get_free(&hg);
    return rc;
}

static void ip_init(InputPlugin* ip, char *filename)
{
    const InputPlugin t = {
        .httpCode           = -1,
        .pcmConvertScale    = -1,
        .duration           = -1,
        .bitrate            = -1,
        .data = {
            .fd         = -1,
            .fileName   = filename,
            .remote     = is_http_url(filename),
            .channelMap = CHANNEL_MAP_INIT
        }
    };
    *ip = t;
}

static void ip_reset(InputPlugin* ip, int close_fd)
{
    int fd = ip->data.fd;
    free(ip->data.metaData);
    ip_init(ip, ip->data.fileName);
    if (fd != -1) {
        if (close_fd)
            close(fd);
        else {
            lseek(fd, 0, SEEK_SET);
            ip->data.fd = fd;
        }
    }
}

static int open_file_locked (InputPlugin* ip)
{
    const InputPluginOps* ops;
//    struct list_head *head = &ip_head;
    const char *ext;
    int rc = 0;

    ext = get_extension(ip->data.fileName);
    if (!ext)
        return -INPUT_ERROR_UNRECOGNIZED_FILE_TYPE;

    ops = get_ops_by_extension(ext);
    if (!ops)
        return -INPUT_ERROR_UNRECOGNIZED_FILE_TYPE;

    ip->data.fd = open(ip->data.fileName, O_RDONLY);
    if (ip->data.fd == -1)
        return -INPUT_ERROR_ERRNO;

    while (1) {
        ip->ops = ops;
        rc = ip->ops->Open(&ip->data);
        if (rc != -INPUT_ERROR_UNSUPPORTED_FILE_TYPE)
            break;

        ops = get_ops_by_extension(ext);
        if (!ops)
            break;

        ip_reset(ip, 0);
        DEBUG ("fallback: try next plugin for `%s'", ip->data.fileName);
    }

    return rc;
}

static int open_file(InputPlugin* ip)
{
    ip_rdlock();
    int rv = open_file_locked(ip);
    ip_unlock();
    return rv;
}

static int sort_ip(const struct list_head *a_, const struct list_head *b_)
{
    const Input* a = list_entry(a_, Input, node);
    const Input* b = list_entry(b_, Input, node);
    return b->priority - a->priority;
}

void ip_load_plugins(void)
{
    DIR *dir;
    struct dirent *d;

    pluginDir = xstrjoin(libDir, "/ip");
    dir = opendir(pluginDir);
    if (dir == NULL) {
        ERROR ("couldn't open directory `%s': %s", pluginDir, strerror(errno));
        return;
    }

    ip_wrlock();
    while (NULL != (d = (struct dirent *) readdir (dir))) {
        char filename[512] = {0};
        Input* ip;
        void *so;
        char *ext;
        const int *priority_ptr;
        const unsigned *abi_version_ptr;
        bool err = false;

        if (d->d_name[0] == '.') {
            continue;
        }

        ext = strrchr(d->d_name, '.');
        if (ext == NULL) {
            DEBUG ("ignore '%s/%s'", pluginDir, d->d_name);
            continue;
        }

        if (0 != g_strcmp0 (ext, ".so")) {
            DEBUG ("ignore '%s/%s'", pluginDir, d->d_name);
            continue;
        }

        snprintf(filename, sizeof(filename), "%s/%s", pluginDir, d->d_name);

        DEBUG ("load plugin: '%s'", filename);

        so = dlopen(filename, RTLD_NOW);
        if (so == NULL) {
            ERROR ("open '%s' error, '%s'", filename, dlerror());
            continue;
        }

        ip = xnew (Input, 1);

        // FIXME:// DJ- 这些符号已废弃
//        abi_version_ptr = dlsym(so, "ip_abi_version");
//        priority_ptr = dlsym(so, "ip_priority");
//        ip->extensions = dlsym(so, "ip_extensions");
//        ip->mimeTypes = dlsym(so, "ip_mime_types");
//        ip->ops = dlsym(so, "ip_ops");
//        ip->options = dlsym(so, "ip_options");
//        if (!priority_ptr || !ip->extensions || !ip->mimeTypes || !ip->ops || !ip->options) {
//            ERROR ("%s: missing symbol", filename);
//            err = true;
//        }
//        if (!abi_version_ptr || *abi_version_ptr != INPUT_ABI_VERSION) {
//            ERROR ("%s: incompatible plugin version", filename);
//            err = true;
//        }
//        if (err) {
//            free(ip);
//            dlclose(so);
//            continue;
//        }
//        ip->priority = *priority_ptr;
//
//        ip->name = xstrndup(d->d_name, ext - d->d_name);
//        ip->handle = so;
//
//        list_add_tail(&ip->node, &ip_head);
    }

//    list_mergesort(&ip_head, sort_ip);

    closedir(dir);

    ip_unlock();
}

InputPlugin* ip_new(const char *filename)
{
    InputPlugin* ip = xnew(InputPlugin, 1);

    ip_init(ip, xstrdup(filename));
    return ip;
}

void ip_delete(InputPlugin* ip)
{
    if (ip->open)
        ip_close(ip);
    free(ip->data.fileName);
    free(ip);
}

int ip_open(InputPlugin* ip)
{
    int rc;

    BUG_ON(ip->open);

    /* set fd and ops, call ops->open */
    if (ip->data.remote) {
        rc = open_remote(ip);
        if (rc == 0)
            rc = ip->ops->Open(&ip->data);
    } else {
        if (is_cdda_url(ip->data.fileName)) {
            ip->ops = get_ops_by_mime_type("x-content/audio-cdda");
            rc = ip->ops ? ip->ops->Open(&ip->data) : 1;
        } else if (is_cue_url(ip->data.fileName)) {
            ip->ops = get_ops_by_mime_type("application/x-cue");
            rc = ip->ops ? ip->ops->Open(&ip->data) : 1;
        } else
            rc = open_file(ip);
    }

    if (rc) {
        DEBUG ("opening `%s' failed: %d %s\n", ip->data.fileName, rc, rc == -1 ? strerror(errno) : "");
        ip_reset(ip, 1);
        return rc;
    }
    ip->open = 1;
    return 0;
}

void ip_setup(InputPlugin* ip)
{
    unsigned int bits, is_signed, channels;
    sample_format_t sf = ip->data.sf;

    bits = sf_get_bits(sf);
    is_signed = sf_get_signed(sf);
    channels = sf_get_channels(sf);

    ip->pcmConvertScale = 1;
    ip->PcmConvert = NULL;
    ip->PcmConvertInPlace = NULL;

    if (bits <= 16 && channels <= 2) {
        unsigned int mask = ((bits >> 2) & 4) | (is_signed << 1);

        ip->PcmConvert = pcm_conv[mask | (channels - 1)];
        ip->PcmConvertInPlace = pcm_conv_in_place[mask | sf_get_bigendian(sf)];

        ip->pcmConvertScale = (3 - channels) * (3 - bits / 8);
    }

    d_print("pcm convert: scale=%d convert=%d convert_in_place=%d\n",
            ip->pcmConvertScale,
            ip->PcmConvert != NULL,
            ip->PcmConvertInPlace != NULL);
}

int ip_close(InputPlugin* ip)
{
    int rc;

    rc = ip->ops->Close(&ip->data);
    BUG_ON(ip->data.private);
    if (ip->data.fd != -1)
        close(ip->data.fd);
    free(ip->data.metaData);
    free(ip->data.icyName);
    free(ip->data.icyGenre);
    free(ip->data.icyURL);
    free(ip->httpReason);

    ip_init(ip, ip->data.fileName);

    return rc;
}

int ip_read(InputPlugin* ip, char *buffer, int count)
{
    struct timeval tv;
    fd_set readfds;
    /* 4608 seems to be optimal for mp3s, 4096 for oggs */
    char tmp[8 * 1024];
    char *buf;
    int sample_size;
    int rc;

    BUG_ON(count <= 0);

    FD_ZERO(&readfds);
    FD_SET(ip->data.fd, &readfds);
    /* zero timeout -> return immediately */
    tv.tv_sec = 0;
    tv.tv_usec = 50e3;
    rc = select(ip->data.fd + 1, &readfds, NULL, NULL, &tv);
    if (rc == -1) {
        if (errno == EINTR)
            errno = EAGAIN;
        return -1;
    }
    if (rc == 0) {
        errno = EAGAIN;
        return -1;
    }

    buf = buffer;
    if (ip->pcmConvertScale > 1) {
        /* use tmp buffer for 16-bit mono and 8-bit */
        buf = tmp;
        count /= ip->pcmConvertScale;
        if (count > sizeof(tmp))
            count = sizeof(tmp);
    }

    rc = ip->ops->Read(&ip->data, buf, count);
    if (rc == -1 && (errno == EAGAIN || errno == EINTR)) {
        errno = EAGAIN;
        return -1;
    }
    if (rc <= 0) {
        ip->eof = 1;
        return rc;
    }

    BUG_ON(rc % sf_get_frame_size(ip->data.sf) != 0);

    sample_size = sf_get_sample_size(ip->data.sf);
    if (ip->PcmConvertInPlace != NULL)
        ip->PcmConvertInPlace(buf, rc / sample_size);
    if (ip->PcmConvert != NULL)
        ip->PcmConvert(buffer, tmp, rc / sample_size);
    return rc * ip->pcmConvertScale;
}

int ip_seek(InputPlugin* ip, double offset)
{
    int rc;

    if (ip->data.remote)
        return -INPUT_ERROR_FUNCTION_NOT_SUPPORTED;
    rc = ip->ops->Seek(&ip->data, offset);
    if (rc == 0)
        ip->eof = 0;
    return rc;
}

int ip_read_comments(InputPlugin* ip, KeyValue** comments)
{
    struct keyval *kv = NULL;
    int rc;

    rc = ip->ops->ReadComments(&ip->data, &kv);

    if (ip->data.remote) {
        GROWING_KEY_VALUES(c);

        if (kv) {
            key_value_init(&c, kv);
            key_value_free(kv);
        }

        if (ip->data.icyName && !key_value_get_val_growing(&c, "title"))
            key_value_add(&c, "title", xstrdup(ip->data.icyName));

        if (ip->data.icyGenre && !key_value_get_val_growing(&c, "genre"))
            key_value_add(&c, "genre", xstrdup(ip->data.icyGenre));

        if (ip->data.icyURL && !key_value_get_val_growing(&c, "comment"))
            key_value_add(&c, "comment", xstrdup(ip->data.icyURL));

        key_value_terminate(&c);

        kv = c.keyValues;
    }

    *comments = kv;

    return ip->data.remote ? 0 : rc;
}

int ip_duration(InputPlugin* ip)
{
    if (ip->data.remote)
        return -1;
    if (ip->duration == -1)
        ip->duration = ip->ops->Duration(&ip->data);
    if (ip->duration < 0)
        return -1;
    return ip->duration;
}

int ip_bitrate(InputPlugin* ip)
{
    if (ip->data.remote)
        return -1;
    if (ip->bitrate == -1)
        ip->bitrate = ip->ops->Bitrate(&ip->data);
    if (ip->bitrate < 0)
        return -1;
    return ip->bitrate;
}

int ip_current_bitrate(InputPlugin* ip)
{
    return ip->ops->BitrateCurrent(&ip->data);
}

char *ip_codec(InputPlugin* ip)
{
    if (ip->data.remote)
        return NULL;
    if (!ip->codec)
        ip->codec = ip->ops->Codec(&ip->data);

    return ip->codec;
}

char *ip_codec_profile(InputPlugin* ip)
{
    if (ip->data.remote)
        return NULL;
    if (!ip->codecProfile)
        ip->codecProfile = ip->ops->CodecProfile(&ip->data);
    return ip->codecProfile;
}

sample_format_t ip_get_sf(InputPlugin* ip)
{
    BUG_ON(!ip->open);
    return ip->data.sf;
}

void ip_get_channel_map(InputPlugin* ip, ChannelPosition* channel_map)
{
    BUG_ON(!ip->open);
    channel_map_copy(channel_map, ip->data.channelMap);
}

const char *ip_get_filename(InputPlugin* ip)
{
    return ip->data.fileName;
}

const char *ip_get_metadata(InputPlugin* ip)
{
    BUG_ON(!ip->open);
    return ip->data.metaData;
}

int ip_is_remote(InputPlugin* ip)
{
    return ip->data.remote;
}

int ip_metadata_changed(InputPlugin* ip)
{
    int ret = ip->data.metaDataChanged;

    BUG_ON(!ip->open);
    ip->data.metaDataChanged = 0;
    return ret;
}

int ip_eof(InputPlugin* ip)
{
    BUG_ON(!ip->open);
    return ip->eof;
}

static void option_error(int rc)
{
    char *msg = ip_get_error_msg(NULL, rc, "setting option");
    error_msg("%s", msg);
    free(msg);
}

static void set_ip_option(void *data, const char *val)
{
    const InputPluginOpt* ipo = data;
    int rc;

    rc = ipo->Set(val);
    if (rc)
        option_error(rc);
}

static void get_ip_option(void *data, char *buf, size_t size)
{
    const InputPluginOpt* ipo = data;
    char *val = NULL;

    ipo->Get(&val);
    if (val) {
        strscpy(buf, val, size);
        free(val);
    }
}

static void set_ip_priority(void *data, const char *val)
{
    /* warn only once during the lifetime of the program. */
    static bool warned = false;
    long tmp;
    Input* ip = data;

    if (str_to_int(val, &tmp) == -1 || tmp < 0 || (long)(int)tmp != tmp) {
        error_msg("non-negative integer expected");
        return;
    }
    if (ui_initialized) {
        if (!warned) {
            static const char *msg =
                "Metadata might become inconsistent "
                "after this change. Continue? [y/N]";
            if (yes_no_query("%s", msg) != UI_QUERY_ANSWER_YES) {
                info_msg("Aborted");
                return;
            }
            warned = true;
        }
        INFO ("Run \":update-cache -f\" to refresh the metadata.");
    }

    ip_wrlock();
    ip->priority = (int)tmp;
//    list_mergesort(&ip_head, sort_ip);
    ip_unlock();
}

static void get_ip_priority(void *data, char *val, size_t size)
{
    const Input* ip = data;
    ip_rdlock();
    snprintf(val, size, "%d", ip->priority);
    ip_unlock();
}

void ip_add_options(void)
{
    Input* ip;
    const InputPluginOpt* ipo;
    char key[64];

    ip_rdlock();
//    list_for_each_entry(ip, &ip_head, node) {
//        for (ipo = ip->options; ipo->name; ipo++) {
//            snprintf(key, sizeof(key), "input.%s.%s", ip->name, ipo->name);
//            option_add(xstrdup(key), ipo, get_ip_option, set_ip_option, NULL, 0);
//        }
//        snprintf(key, sizeof(key), "input.%s.priority", ip->name);
//        option_add(xstrdup(key), ip, get_ip_priority, set_ip_priority, NULL, 0);
//    }
    ip_unlock();
}

char *ip_get_error_msg(InputPlugin* ip, int rc, const char *arg)
{
    char buffer[1024];

    switch (-rc) {
        case INPUT_ERROR_ERRNO:
            snprintf(buffer, sizeof(buffer), "%s: %s", arg, strerror(errno));
            break;
        case INPUT_ERROR_UNRECOGNIZED_FILE_TYPE:
            snprintf(buffer, sizeof(buffer),
                     "%s: unrecognized filename extension", arg);
            break;
        case INPUT_ERROR_UNSUPPORTED_FILE_TYPE:
            snprintf(buffer, sizeof(buffer),
                     "%s: unsupported file format", arg);
            break;
        case INPUT_ERROR_FUNCTION_NOT_SUPPORTED:
            snprintf(buffer, sizeof(buffer),
                     "%s: function not supported", arg);
            break;
        case INPUT_ERROR_FILE_FORMAT:
            snprintf(buffer, sizeof(buffer),
                     "%s: file format not supported or corrupted file",
                     arg);
            break;
        case INPUT_ERROR_INVALID_URI:
            snprintf(buffer, sizeof(buffer), "%s: invalid URI", arg);
            break;
        case INPUT_ERROR_SAMPLE_FORMAT:
            snprintf(buffer, sizeof(buffer),
                     "%s: input plugin doesn't support the sample format",
                     arg);
            break;
        case INPUT_ERROR_WRONG_DISC:
            snprintf(buffer, sizeof(buffer), "%s: wrong disc inserted, aborting!", arg);
            break;
        case INPUT_ERROR_NO_DISC:
            snprintf(buffer, sizeof(buffer), "%s: could not read disc", arg);
            break;
        case INPUT_ERROR_HTTP_RESPONSE:
            snprintf(buffer, sizeof(buffer), "%s: invalid HTTP response", arg);
            break;
        case INPUT_ERROR_HTTP_STATUS:
            snprintf(buffer, sizeof(buffer), "%s: %d %s", arg, ip->httpCode, ip->httpReason);
            free(ip->httpReason);
            ip->httpReason = NULL;
            ip->httpCode = -1;
            break;
        case INPUT_ERROR_HTTP_REDIRECT_LIMIT:
            snprintf(buffer, sizeof(buffer), "%s: too many HTTP redirections", arg);
            break;
        case INPUT_ERROR_NOT_OPTION:
            snprintf(buffer, sizeof(buffer),
                     "%s: no such option", arg);
            break;
        case INPUT_ERROR_INTERNAL:
            snprintf(buffer, sizeof(buffer), "%s: internal error", arg);
            break;
        case INPUT_ERROR_SUCCESS:
        default:
            snprintf(buffer, sizeof(buffer),
                     "%s: this is not an error (%d), this is a bug",
                     arg, rc);
            break;
    }
    return xstrdup(buffer);
}

char **ip_get_supported_extensions(void)
{
    Input* ip;
    char **exts;
    int i, size;
    int count = 0;

    size = 8;
    exts = xnew(char *, size);
    ip_rdlock();
//    list_for_each_entry(ip, &ip_head, node) {
//        const char * const *e = ip->extensions;
//
//        for (i = 0; e[i]; i++) {
//            if (count == size - 1) {
//                size *= 2;
//                exts = xrenew(char *, exts, size);
//            }
//            exts[count++] = xstrdup(e[i]);
//        }
//    }
    ip_unlock();
    exts[count] = NULL;
    qsort(exts, count, sizeof(char *), strptrcmp);
    return exts;
}

void ip_dump_plugins (void)
{
    Input* ip;
    int i;

    printf("Input Plugins: %s", pluginDir);
    ip_rdlock();
//    list_for_each_entry(ip, &ip_head, node) {
//        printf("  %s:\n    Priority: %d\n    File Types:", ip->name, ip->priority);
//        for (i = 0; ip->extensions[i]; i++)
//            printf(" %s", ip->extensions[i]);
//        printf("\n    MIME Types:");
//        for (i = 0; ip->mimeTypes[i]; i++)
//            printf(" %s", ip->mimeTypes[i]);
//        printf("\n");
//    }
    ip_unlock();
}
