//
// Created by dingjing on 23-2-20.
//

#ifndef GRACEFUL_MUSIC_CURSES_MAIN_H
#define GRACEFUL_MUSIC_CURSES_MAIN_H
#include <signal.h>
#include <stdbool.h>

typedef enum _UIQueryAnswer                     UIQueryAnswer;
typedef enum _UICursesInputMode                 UICursesInputMode;

enum _UICursesInputMode
{
    NORMAL_MODE,
    COMMAND_MODE,
    SEARCH_MODE
};

enum _UIQueryAnswer
{
    UI_QUERY_ANSWER_ERROR   = -1,
    UI_QUERY_ANSWER_NO      = 0,
    UI_QUERY_ANSWER_YES     = 1
};

extern int                          gUIInitialized;
extern int                          gCurView;
extern int                          gPrevView;
extern UICursesInputMode            gInputMode;
extern volatile sig_atomic_t        gRunning;
extern char*                        gCharset;
extern bool                         gUsingUtf8;

extern struct searchable*           searchable;

extern char*                        lib_filename;
extern char*                        lib_ext_filename;
extern char*                        pl_filename;
extern char*                        play_queue_filename;
extern char*                        play_queue_ext_filename;





#endif //GRACEFUL_MUSIC_CURSES_MAIN_H
