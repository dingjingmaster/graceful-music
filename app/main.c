#include <errno.h>
#include <locale.h>
#include <gio/gio.h>

#include "log.h"
#include "ui_curses.h"


char*               gLogPath = NULL;
char*               gCharset = "UTF-8";

const char*         gHomeDir = NULL;
const char*         gConfigDir = NULL;
const char*         gSocketPath = NULL;
const char*         gRuntimeDir = NULL;

int main (int argc, char* argv[])
{
    gLogPath = g_strdup_printf ("%s/%s.log", g_get_home_dir(), PACKAGE_NAME);
    g_log_set_writer_func (log_handler, NULL, NULL);

    setlocale (LC_ALL, "zh_CN.UTF-8");

    INFO ("%s starting ...", PACKAGE_NAME)

    gHomeDir = g_get_home_dir();
    if (gHomeDir == NULL) {
        DIE ("environment variable 'HOME' not set")
    }

    gRuntimeDir = g_get_user_runtime_dir();
    if (gRuntimeDir == NULL) {
        DIE ("environment variable 'XDG_RUNTIME_DIR' not set")
    }

    char* configDir = g_get_user_config_dir();
    if (configDir == NULL) {
        DIE ("environment variable 'XDG_CONFIG_DIR' not set")
    }

    gConfigDir = g_strdup_printf ("%s/graceful-music", configDir);
    if (g_mkdir_with_parents (gConfigDir, 0755)) {
        DIE ("create config dir: '%s' error, '%s'", gConfigDir, g_strerror (errno));
    }

    gSocketPath = g_strdup_printf ("%s/graceful-music.sock", gRuntimeDir);
    if (!gSocketPath) {
        DIE ("get socket path: '%s' error.", gSocketPath);
    }

    curses_main (argc, argv);

    return 0;
}