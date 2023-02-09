/*************************************************************************
> FileName: main.c
> Author  : DingJing
> Mail    : dingjing@live.cn
> Created Time: Thu 09 Feb 2023 22:09:10 PM CST
 ************************************************************************/

#include <gio/gio.h>

#include "log.h"
#include "ui_curses.h"


char*               gLogPath = NULL;

int main (int argc, char* argv[])
{
    gLogPath = g_strdup_printf ("%s/%s.log", g_get_home_dir(), PACKAGE_NAME);

    g_log_set_writer_func (log_handler, NULL, NULL);

    curses_main (argc, argv);

    return 0;
}