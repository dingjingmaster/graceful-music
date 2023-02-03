//
// Created by dingjing on 2/3/23.
//

/**
 * @brief
 *  控制播放的所有功能封装，预计会有 ncurses 界面 + gtk 界面，最终都会调用这里的函数
 */

#ifndef GRACEFUL_MUSIC_PLAYER_CONTROL_H
#define GRACEFUL_MUSIC_PLAYER_CONTROL_H
#include "track-info.h"

typedef enum _FileType          FileType;

enum _FileType
{
    FILE_TYPE_INVALID,

    FILE_TYPE_URL,
    FILE_TYPE_DIR,
    FILE_TYPE_FILE,
};

int         pc_init             (void);
int         pc_exit             (void);
void        pc_play_file        (const char* fileName);



#endif //GRACEFUL_MUSIC_PLAYER_CONTROL_H
