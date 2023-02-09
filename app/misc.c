#include "misc.h"
#include "prog.h"
#include "xmalloc.h"
#include "xstrjoin.h"
#include "ui_curses.h"
#include "log.h"
#include "global.h"


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdarg.h>
#include <pwd.h>

#define DATADIR     "/usr/share"
#define LIBDIR      "/usr/lib"

const char*     dataDir = NULL;
const char*     libDir = NULL;

char **get_words(const char *text)
{
    char **words;
    int i, j, count;

    while (*text == ' ')
        text++;

    count = 0;
    i = 0;
    while (text[i]) {
        count++;
        while (text[i] && text[i] != ' ')
            i++;
        while (text[i] == ' ')
            i++;
    }
    words = xnew(char *, count + 1);

    i = 0;
    j = 0;
    while (text[i]) {
        int start = i;

        while (text[i] && text[i] != ' ')
            i++;
        words[j++] = xstrndup(text + start, i - start);
        while (text[i] == ' ')
            i++;
    }
    words[j] = NULL;
    return words;
}

int strptrcmp(const void *a, const void *b)
{
    const char *as = *(char **)a;
    const char *bs = *(char **)b;

    return strcmp(as, bs);
}

int strptrcoll(const void *a, const void *b)
{
    const char *as = *(char **)a;
    const char *bs = *(char **)b;

    return strcoll(as, bs);
}

const char *escape(const char *str)
{
    static char *buf = NULL;
    static size_t alloc = 0;
    size_t len = strlen(str);
    size_t need = len * 2 + 1;
    int s, d;

    if (need > alloc) {
        alloc = (need + 16) & ~(16 - 1);
        buf = xrealloc(buf, alloc);
    }

    d = 0;
    for (s = 0; str[s]; s++) {
        if (str[s] == '\\') {
            buf[d++] = '\\';
            buf[d++] = '\\';
            continue;
        }
        if (str[s] == '\n') {
            buf[d++] = '\\';
            buf[d++] = 'n';
            continue;
        }
        buf[d++] = str[s];
    }
    buf[d] = 0;
    return buf;
}

const char *unescape(const char *str)
{
    static char *buf = NULL;
    static size_t alloc = 0;
    size_t need = strlen(str) + 1;
    int do_escape = 0;
    int s, d;

    if (need > alloc) {
        alloc = (need + 16) & ~(16 - 1);
        buf = xrealloc(buf, alloc);
    }

    d = 0;
    for (s = 0; str[s]; s++) {
        if (!do_escape && str[s] == '\\')
            do_escape = 1;
        else {
            buf[d++] = (do_escape && str[s] == 'n') ? '\n' : str[s];
            do_escape = 0;
        }
    }
    buf[d] = 0;
    return buf;
}

static int dir_exists(const char *dirname)
{
    DIR *dir;

    dir = opendir(dirname);
    if (dir == NULL) {
        if (errno == ENOENT)
            return 0;
        return -1;
    }
    closedir(dir);
    return 1;
}

static void make_dir(const char *dirname)
{
    int rc;

    rc = dir_exists(dirname);
    if (rc == 1)
        return;
    if (rc == -1)
        die_errno("error: opening `%s'", dirname);
    rc = mkdir(dirname, 0700);
    if (rc == -1)
        die_errno("error: creating directory `%s'", dirname);
}

static char *get_non_empty_env(const char *name)
{
    const char *val;

    val = getenv(name);
    if (val == NULL || val[0] == 0)
        return NULL;
    return xstrdup(val);
}

const char *get_filename(const char *path)
{
    const char *file = strrchr(path, '/');
    if (!file)
        file = path;
    else
        file++;
    if (!*file)
        return NULL;
    return file;
}

static void move_old_playlist(void)
{
    char *default_playlist = xstrjoin(gPlaylistDir, "/default");
    char *old_playlist = xstrjoin(gConfigDir, "/playlist.pl");
    int rc = rename(old_playlist, default_playlist);
    if (rc && errno != ENOENT)
        die_errno("error: unable to move %s to playlist directory",
                  old_playlist);
    free(default_playlist);
    free(old_playlist);
}

int misc_init(void)
{
    int playlistDirIsNew = dir_exists(gPlaylistDir) == 0;
    make_dir(gPlaylistDir);
    if (playlistDirIsNew) {
        move_old_playlist();
    }


    // FIXME:// 去掉
    libDir = LIBDIR "/cmus";
    dataDir = DATADIR "/cmus";

    return 0;
}

int replaygain_decode(unsigned int field, int *gain)
{
    unsigned int name_code, originator_code, sign_bit, val;

    name_code = (field >> 13) & 0x7;
    if (!name_code || name_code > 2)
        return 0;
    originator_code = (field >> 10) & 0x7;
    if (!originator_code)
        return 0;
    sign_bit = (field >> 9) & 0x1;
    val = field & 0x1ff;
    if (sign_bit && !val)
        return 0;
    *gain = (sign_bit ? -1 : 1) * val;
    return name_code;
}

static char *get_home_dir(const char *username)
{
    struct passwd *passwd;

    if (username == NULL)
        return xstrdup(gHomeDir);
    passwd = getpwnam(username);
    if (passwd == NULL)
        return NULL;
    /* don't free passwd */
    return xstrdup(passwd->pw_dir);
}

char *expand_filename(const char *name)
{
    if (name[0] == '~') {
        char *slash;

        slash = strchr(name, '/');
        if (slash) {
            char *username, *home;

            if (slash - name - 1 > 0) {
                /* ~user/... */
                username = xstrndup(name + 1, slash - name - 1);
            } else {
                /* ~/... */
                username = NULL;
            }
            home = get_home_dir(username);
            free(username);
            if (home) {
                char *expanded;

                expanded = xstrjoin(home, slash);
                free(home);
                return expanded;
            } else {
                return xstrdup(name);
            }
        } else {
            if (name[1] == 0) {
                return xstrdup(gHomeDir);
            } else {
                char *home;

                home = get_home_dir(name + 1);
                if (home)
                    return home;
                return xstrdup(name);
            }
        }
    } else {
        return xstrdup(name);
    }
}

void shuffle_array(void *array, size_t n, size_t size)
{
    char tmp[size];
    char *arr = array;
    for (ssize_t i = 0; i < (ssize_t)n - 1; ++i) {
        size_t rnd = (size_t) rand();
        size_t j = i + rnd / (RAND_MAX / (n - i) + 1);
        memcpy(tmp, arr + j * size, size);
        memcpy(arr + j * size, arr + i * size, size);
        memcpy(arr + i * size, tmp, size);
    }
}
