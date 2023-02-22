//
// Created by dingjing on 23-2-21.
//

#include "main-window.h"

int main (int argc, char* argv[])
{
    int ret = 0;

    ret = main_window_init (argc, argv);

    main_window_loop ();

    ret = main_window_exit();

    return ret;
}