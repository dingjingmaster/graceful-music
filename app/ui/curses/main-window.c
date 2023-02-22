//
// Created by dingjing on 23-2-21.
//

#include "main-window.h"

#include <stdbool.h>
#include <ncurses.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <bits/types/sig_atomic_t.h>

static int                      gWinW = 0;
static bool                     gRunning = true;
static volatile sig_atomic_t    gNeedsToResize = 0;


static void update_window_size (void);
static int get_window_size (int* w, int* h);            // ok! 获取终端大小


int main_window_init (int argc, char **argv)
{
    initscr ();
    nodelay (stdscr, TRUE);             // 初始化
    keypad (stdscr, TRUE);              // 允许按键映射
    halfdelay (5);
    noecho();

    return 0;
}

void main_window_loop ()
{
    int rc = 0;
    int fdHigh = 0;

#define SELECT_ADD_FD(fd) do {  \
	FD_SET((fd), &set);         \
	if ((fd) > fdHigh) {        \
        fdHigh = (fd);          \
    }                           \
} while(0)

    while (gRunning) {
        fd_set              set;
        struct timeval      tv;
        int                 pollMixer = 0;

        FD_ZERO(&set);
        SELECT_ADD_FD(0);

        rc = select (fdHigh + 1, &set, NULL, NULL, tv.tv_usec ? &tv : NULL);
        if (pollMixer) {

        }

        if (rc <= 0) {
            continue;
        }


    }
}

int main_window_exit ()
{
    int ret = endwin();

    return 0;
}

void main_window_update ()
{
    if (gNeedsToResize) {
        //
        update_window_size ();
    }
}

static void update_window_size (void)
{
    int w = 0, h = 0;
    int c = 0, l = 0;

    if (0 == get_window_size (&c, &l)) {
        gNeedsToResize = false;
        resizeterm (c, l);
        gWinW = w = COLS < 3 ? 3 : COLS;
        h = ((LINES - 3) < 2) ? 2 : (LINES - 3);
    }
}

static int get_window_size (int* w, int* h)
{
    struct winsize  ws;

    if (-1 == ioctl (0, TIOCGWINSZ, &ws)) {
        return -1;
    }

    *w = ws.ws_row;
    *h = ws.ws_col;

    return 0;
}
