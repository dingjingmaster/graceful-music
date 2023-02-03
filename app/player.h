//
// Created by dingjing on 1/31/23.
//

#ifndef GRACEFUL_MUSIC_PLAYER_H
#define GRACEFUL_MUSIC_PLAYER_H

#include "track-info.h"

typedef enum _PlayerStatus          PlayerStatus;
typedef enum _ReplayGain            ReplayGain;
typedef struct _PlayerInfo          PlayerInfo;

enum
{
    PLAYER_ERROR_SUCCESS,           // 无错误
    PLAYER_ERROR_ERRNO,             // 系统错误
    PLAYER_ERROR_NOT_SUPPORTED      // 不支持
};

enum _PlayerStatus
{
    PLAYER_STATUS_STOPPED,
    PLAYER_STATUS_PLAYING,
    PLAYER_STATUS_PAUSED,
    NR_PLAYER_STATUS
};

enum _ReplayGain
{
    RG_DISABLED,
    RG_TRACK,
    RG_ALBUM,
    RG_TRACK_PREFERRED,
    RG_ALBUM_PREFERRED,
    RG_SMART
};

struct _PlayerInfo
{
    TrackInfo*                      ti;
    PlayerStatus                    status;

    int                             pos;
    int                             currentBitrate;

    int                             bufferFill;
    int                             bufferSize;

    char*                           errorMsg;

    unsigned int                    fileChanged : 1;
    unsigned int                    metaDataChanged : 1;
    unsigned int                    statusChanged : 1;
    unsigned int                    positionChanged : 1;
    unsigned int                    bufferFillChanged : 1;
};


void player_init(void);
void player_exit(void);

void player_set_file    (TrackInfo* ti);
void player_play_file   (TrackInfo* ti);


#endif //GRACEFUL_MUSIC_PLAYER_H
