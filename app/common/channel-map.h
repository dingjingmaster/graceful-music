//
// Created by dingjing on 2/2/23.
//

#ifndef GRACEFUL_MUSIC_CHANNEL_MAP_H
#define GRACEFUL_MUSIC_CHANNEL_MAP_H
#include <string.h>

/**
 * @brief
 *  声道设置
 */

#define CHANNELS_MAX                    (32)
#define CHANNEL_MAP_INIT                {CHANNEL_POSITION_INVALID}
#define CHANNEL_MAP(name)               ChannelPosition name[CHANNELS_MAX] = CHANNEL_MAP_INIT

typedef enum _ChannelPosition           ChannelPosition;

enum _ChannelPosition
{
    CHANNEL_POSITION_INVALID = -1,
    CHANNEL_POSITION_MONO = 0,
    CHANNEL_POSITION_FRONT_LEFT,
    CHANNEL_POSITION_FRONT_RIGHT,
    CHANNEL_POSITION_FRONT_CENTER,

    CHANNEL_POSITION_LEFT = CHANNEL_POSITION_FRONT_LEFT,
    CHANNEL_POSITION_RIGHT = CHANNEL_POSITION_FRONT_RIGHT,
    CHANNEL_POSITION_CENTER = CHANNEL_POSITION_FRONT_CENTER,

    CHANNEL_POSITION_REAR_CENTER,
    CHANNEL_POSITION_REAR_LEFT,
    CHANNEL_POSITION_REAR_RIGHT,

    CHANNEL_POSITION_LFE,
    CHANNEL_POSITION_SUBWOOFER = CHANNEL_POSITION_LFE,

    CHANNEL_POSITION_FRONT_LEFT_OF_CENTER,
    CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER,

    CHANNEL_POSITION_SIDE_LEFT,
    CHANNEL_POSITION_SIDE_RIGHT,

    CHANNEL_POSITION_TOP_CENTER,

    CHANNEL_POSITION_TOP_FRONT_LEFT,
    CHANNEL_POSITION_TOP_FRONT_RIGHT,
    CHANNEL_POSITION_TOP_FRONT_CENTER,

    CHANNEL_POSITION_TOP_REAR_LEFT,
    CHANNEL_POSITION_TOP_REAR_RIGHT,
    CHANNEL_POSITION_TOP_REAR_CENTER,

    CHANNEL_POSITION_MAX
};

static inline int channel_map_valid (const ChannelPosition* map)
{
    return map[0] != CHANNEL_POSITION_INVALID;
}

static inline int channel_map_equal (const ChannelPosition* a, const ChannelPosition* b, int channels)
{
    return (0 == memcmp (a, b, sizeof (*a) * channels));
}

static inline ChannelPosition* channel_map_copy (ChannelPosition* dst, const ChannelPosition* src)
{
    return memcpy (dst, src, sizeof (*dst) * CHANNELS_MAX);
}

static inline void channel_map_init_stereo (ChannelPosition* map)
{
    map[0] = CHANNEL_POSITION_LEFT;
    map[1] = CHANNEL_POSITION_RIGHT;
}

void channel_map_init_wave_ex (int channels, unsigned int mask, ChannelPosition* map);


#endif //GRACEFUL_MUSIC_CHANNEL_MAP_H
