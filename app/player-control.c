//
// Created by dingjing on 2/3/23.
//

#include "player-control.h"

#include "cache.h"
//#include ""

static char**           gPlayableMusicType = NULL;

int pc_init (void)
{
    // FIXME:// 获取可播放的音乐类型
    cache_init();
}

int pc_exit (void);

void pc_play_file (const char* fileName);
