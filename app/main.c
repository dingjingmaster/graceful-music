#include <errno.h>
#include <locale.h>
#include <stdbool.h>
#include <gio/gio.h>

#include "log.h"
#include "global.h"
#include "ui_curses.h"


char*               gLogPath = NULL;
char*               gCharset = "UTF-8";

const char*         gHomeDir = NULL;
const char*         gConfigDir = NULL;
const char*         gSocketPath = NULL;
const char*         gRuntimeDir = NULL;
const char*         gPlaylistDir = NULL;

InputPlugins*       gInputPlugins = NULL;

/**
 * @brief
 *  全局变量初始化，中途有任何变量初始化失败都会输出日志并导致程序退出。
 */
void global_value_init ();


int main (int argc, char* argv[])
{
    setlocale (LC_ALL, "zh_CN.UTF-8");

    global_value_init ();

    g_log_set_writer_func (log_handler, NULL, NULL);

    INFO ("%s starting ...", PACKAGE_NAME)

    /**
     * NOTE:// 多个界面
     */
    curses_main (argc, argv);

    return 0;
}

void global_value_init ()
{
    gHomeDir = g_get_home_dir();
    if (gHomeDir == NULL) {
        DIE_BEFORE_LOG_INIT ("environment variable 'HOME' not set")
    }

    const char* runtimeDir = g_get_user_runtime_dir();
    if (runtimeDir == NULL) {
        DIE_BEFORE_LOG_INIT ("environment variable 'XDG_RUNTIME_DIR' not set")
    }
    gRuntimeDir = g_strdup_printf ("%s/graceful-music", runtimeDir);
    if (g_mkdir_with_parents (gRuntimeDir, 0755)) {
        DIE_BEFORE_LOG_INIT ("create runtime dir: '%s' error, '%s'", gRuntimeDir, g_strerror (errno));
    }

    gLogPath = g_strdup_printf ("%s/graceful-music.log", gRuntimeDir);
    if (!gLogPath) {
        DIE_BEFORE_LOG_INIT ("get log path: '%s' error.", gLogPath);
    }

    gSocketPath = g_strdup_printf ("%s/graceful-music.sock", gRuntimeDir);
    if (!gSocketPath) {
        DIE_BEFORE_LOG_INIT ("get socket path: '%s' error.", gSocketPath);
    }


    const char* configDir = g_get_user_config_dir();
    if (configDir == NULL) {
        DIE_BEFORE_LOG_INIT ("environment variable 'XDG_CONFIG_DIR' not set")
    }

    gConfigDir = g_strdup_printf ("%s/graceful-music", configDir);
    if (g_mkdir_with_parents (gConfigDir, 0755)) {
        DIE_BEFORE_LOG_INIT ("create config dir: '%s' error, '%s'", gConfigDir, g_strerror (errno));
    }

    gPlaylistDir = g_strdup_printf ("%s/playlist", gConfigDir);
    if (!gPlaylistDir) {
        DIE_BEFORE_LOG_INIT ("get playlist dir: '%s' error '%s'", gPlaylistDir, g_strerror (errno));
    }

    if (g_mkdir_with_parents (gPlaylistDir, 0755)) {
        DIE_BEFORE_LOG_INIT ("create config dir: '%s' error, '%s'", gPlaylistDir, g_strerror (errno));
    }
}
