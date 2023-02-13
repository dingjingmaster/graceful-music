//
// Created by dingjing on 2/13/23.
//

#ifndef GRACEFUL_MUSIC_FFMPEG_H
#define GRACEFUL_MUSIC_FFMPEG_H

#include "input-interface.h"

typedef struct _FfmpegInput             FfmpegInput;
typedef struct _FfmpegOutput            FfmpegOutput;
typedef struct _FfmpegPrivate           FfmpegPrivate;


void ffmpeg_input_register (InputPlugin* plugin);


#endif //GRACEFUL_MUSIC_FFMPEG_H
