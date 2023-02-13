//
// Created by dingjing on 2/13/23.
//

#ifndef GRACEFUL_MUSIC_AAC_H
#define GRACEFUL_MUSIC_AAC_H

#include "input-interface.h"

#include "channel-map.h"

#include <neaacdec.h>

static inline ChannelPosition channel_position_aac (unsigned char c)
{
    switch (c) {
        case FRONT_CHANNEL_CENTER:  return CHANNEL_POSITION_FRONT_CENTER;
        case FRONT_CHANNEL_LEFT:    return CHANNEL_POSITION_FRONT_LEFT;
        case FRONT_CHANNEL_RIGHT:   return CHANNEL_POSITION_FRONT_RIGHT;
        case SIDE_CHANNEL_LEFT:	    return CHANNEL_POSITION_SIDE_LEFT;
        case SIDE_CHANNEL_RIGHT:    return CHANNEL_POSITION_SIDE_RIGHT;
        case BACK_CHANNEL_LEFT:     return CHANNEL_POSITION_REAR_LEFT;
        case BACK_CHANNEL_RIGHT:    return CHANNEL_POSITION_REAR_RIGHT;
        case BACK_CHANNEL_CENTER:   return CHANNEL_POSITION_REAR_CENTER;
        case LFE_CHANNEL:           return CHANNEL_POSITION_LFE;
        default:        	        return CHANNEL_POSITION_INVALID;
    }
}

void aac_input_register (InputPlugin* plugin);


#endif //GRACEFUL_MUSIC_AAC_H
