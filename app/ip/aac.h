#ifndef CMUS_AAC_H
#define CMUS_AAC_H

#include "../channelmap.h"
#include <neaacdec.h>

static inline channel_position_t channel_position_aac(unsigned char c)
{
	switch (c) {
	case FRONT_CHANNEL_CENTER:	return CHANNEL_POSITION_FRONT_CENTER;
	case FRONT_CHANNEL_LEFT:	return CHANNEL_POSITION_FRONT_LEFT;
	case FRONT_CHANNEL_RIGHT:	return CHANNEL_POSITION_FRONT_RIGHT;
	case SIDE_CHANNEL_LEFT:		return CHANNEL_POSITION_SIDE_LEFT;
	case SIDE_CHANNEL_RIGHT:	return CHANNEL_POSITION_SIDE_RIGHT;
	case BACK_CHANNEL_LEFT:		return CHANNEL_POSITION_REAR_LEFT;
	case BACK_CHANNEL_RIGHT:	return CHANNEL_POSITION_REAR_RIGHT;
	case BACK_CHANNEL_CENTER:	return CHANNEL_POSITION_REAR_CENTER;
	case LFE_CHANNEL:		return CHANNEL_POSITION_LFE;
	default:			return CHANNEL_POSITION_INVALID;
	}
}

#endif
