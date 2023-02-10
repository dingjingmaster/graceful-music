//
// Created by dingjing on 2/2/23.
//

#include "channel-map.h"

/* http://www.microsoft.com/whdc/device/audio/multichaud.mspx#EMLAC */
static const ChannelPosition gChannelMapWaveEX [] = {
    CHANNEL_POSITION_FRONT_LEFT,
    CHANNEL_POSITION_FRONT_RIGHT,
    CHANNEL_POSITION_FRONT_CENTER,
    CHANNEL_POSITION_LFE,
    CHANNEL_POSITION_REAR_LEFT,
    CHANNEL_POSITION_REAR_RIGHT,
    CHANNEL_POSITION_FRONT_LEFT_OF_CENTER,
    CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER,
    CHANNEL_POSITION_REAR_CENTER,
    CHANNEL_POSITION_SIDE_LEFT,
    CHANNEL_POSITION_SIDE_RIGHT,
    CHANNEL_POSITION_TOP_CENTER,
    CHANNEL_POSITION_TOP_FRONT_LEFT,
    CHANNEL_POSITION_TOP_FRONT_CENTER,
    CHANNEL_POSITION_TOP_FRONT_RIGHT,
    CHANNEL_POSITION_TOP_REAR_LEFT,
    CHANNEL_POSITION_TOP_REAR_CENTER,
    CHANNEL_POSITION_TOP_REAR_RIGHT
};

void channel_map_init_wave_ex (int channels, unsigned int mask, ChannelPosition* map)
{
    int num = sizeof (gChannelMapWaveEX) / sizeof (gChannelMapWaveEX[0]);
    if (1 == channels) {
        map [0] = CHANNEL_POSITION_MONO;
    }
    else if (channels > 1 && channels < num) {
        int i = 0;
        int j = 0;

        if (!mask) {
            mask = (1 << channels) - 1;
        }

        for (i = 0, j = 0; i < num; ++i) {
            if (mask & (1 << i)) {
                map[++j] = gChannelMapWaveEX[i];
            }
        }

        if (j != channels) {
            map[0] = CHANNEL_POSITION_INVALID;
        }
    }
    else {
        map[0] = CHANNEL_POSITION_INVALID;
    }
}
