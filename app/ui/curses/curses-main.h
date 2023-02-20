//
// Created by dingjing on 23-2-20.
//

#ifndef GRACEFUL_MUSIC_CURSES_MAIN_H
#define GRACEFUL_MUSIC_CURSES_MAIN_H
#include "search.h"
#include "format-print.h"

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

extern Searchable*                  gSearchable;

extern char*                        gLibFilename;
extern char*                        gLibExtFilename;
extern char*                        gPlaylistFilename;
extern char*                        gPlayQueueFilename;
extern char*                        gPlayQueueExtFilename;


int curses_main(int argc, char *argv[]);

void update_title_line(void);
void update_status_line(void);
void update_filter_line(void);
void update_full(void);
void update_size(void);
void update_colors(void);
void info_msg(const char *format, ...);
void error_msg(const char *format, ...);
UIQueryAnswer yes_no_query(const char *format, ...);
void search_not_found(void);
void set_view(int view);
void set_client_fd(int fd);
int get_client_fd(void);
void enter_search_mode(void);
void enter_command_mode(void);
void enter_search_backward_mode(void);

int track_format_valid(const char *format);

/* lock player_info ! */
const char *get_stream_title(void);
const FormatOption* get_global_format_options(void);

int get_track_win_x (void);





#endif //GRACEFUL_MUSIC_CURSES_MAIN_H
