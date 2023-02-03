//
// Created by dingjing on 2/2/23.
//

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gio/gio.h>

#include "log.h"

char* gLogPath = NULL;
const char* gMusicDataDir = NULL;
const char* gMusicConfigPath = NULL;
const char* gMusicSocketPath = NULL;

int main (int argc, char* argv[])
{
    // 初始化必须的全局变量
    gMusicDataDir = g_strdup_printf ("%s/graceful-music/", g_getenv ("XDG_RUNTIME_DIR"));
    gMusicSocketPath = g_strdup_printf ("%s/graceful-music.sock", gMusicDataDir);
    gMusicConfigPath = g_strdup_printf ("%s/.config/graceful-music.conf", g_getenv ("HOME"));
    gLogPath = g_strdup_printf ("%s/graceful-music.log", gMusicDataDir);

    if (gMusicDataDir) {
        g_autoptr(GFile) f = g_file_new_for_path (gMusicDataDir);
        if (!g_file_query_exists (f, NULL)) {
            g_mkdir_with_parents (gMusicDataDir, 0750);
        }
    }

    // 日志
    g_log_set_writer_func (log_handler, NULL, NULL);

    INFO("\n%s start running...", PACKAGE_NAME);

    DEBUG("data dir: %s", gMusicDataDir);
    DEBUG("config path: %s", gMusicConfigPath);
    DEBUG("socket path: %s", gMusicSocketPath);

    INFO("%s exited!\n", PACKAGE_NAME);

    return 0;
}