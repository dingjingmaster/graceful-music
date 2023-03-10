#ifndef CMUS_CHANNELMAP_H
#define CMUS_CHANNELMAP_H

#include <string.h>

#define CHANNELS_MAX 32

/* Modelled after PulseAudio */
enum channel_position {
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

typedef enum channel_position	channel_position_t;

#define CHANNEL_MAP_INIT	{ CHANNEL_POSITION_INVALID }

#define CHANNEL_MAP(name) \
	channel_position_t name[CHANNELS_MAX] = CHANNEL_MAP_INIT

static inline int channel_map_valid(const channel_position_t *map)
{
	return map[0] != CHANNEL_POSITION_INVALID;
}

static inline int channel_map_equal(const channel_position_t *a, const channel_position_t *b, int channels)
{
	return memcmp(a, b, sizeof(*a) * channels) == 0;
}

static inline channel_position_t *channel_map_copy(channel_position_t *dst, const channel_position_t *src)
{
	return memcpy(dst, src, sizeof(*dst) * CHANNELS_MAX);
}

static inline void channel_map_init_stereo(channel_position_t *map)
{
	map[0] = CHANNEL_POSITION_LEFT;
	map[1] = CHANNEL_POSITION_RIGHT;
}

void channel_map_init_waveex(int channels, unsigned int mask, channel_position_t *map);

#endif
