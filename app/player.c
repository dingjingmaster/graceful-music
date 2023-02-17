#include "player.h"
#include "buffer.h"
#include "input.h"
#include "output.h"
#include "sf.h"
#include "utils.h"
#include "xmalloc.h"
#include "log.h"
#include "compiler.h"
#include "options.h"
#include "mpris.h"
#include "lib.h"
#include "graceful-music.h"
#include "plugins/output-interface.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <stdarg.h>
#include <math.h>

const char * const player_status_names[] = {
	"stopped", "playing", "paused", NULL
};

enum producer_status {
	PS_UNLOADED,
	PS_STOPPED,
	PS_PLAYING,
	PS_PAUSED
};

enum consumer_status {
	CS_STOPPED,
	CS_PLAYING,
	CS_PAUSED
};

/* protects player_info_priv and player_metadata */
static pthread_mutex_t player_info_mutex = GM_MUTEX_INITIALIZER;
struct player_info player_info;
char player_metadata[255 * 16 + 1];
static struct player_info player_info_priv = {
	.ti = NULL,
	.status = PLAYER_STATUS_STOPPED,
	.pos = 0,
	.current_bitrate = -1,
	.buffer_fill = 0,
	.buffer_size = 0,
	.error_msg = NULL,
	.file_changed = 0,
	.metadata_changed = 0,
	.status_changed = 0,
	.position_changed = 0,
	.buffer_fill_changed = 0,
};

/* continue playing after track is finished? */
int player_cont = 1;

/* continue playing after album is finished? */
int player_cont_album = 1;

/* repeat current track forever? */
int player_repeat_current;

enum replaygain replaygain;
int replaygain_limit = 1;
double replaygain_preamp = 0.0;

int soft_vol;
int soft_vol_l;
int soft_vol_r;

static sample_format_t buffer_sf;
static CHANNEL_MAP(buffer_channel_map);

static pthread_t gProducerThread;
static pthread_mutex_t producer_mutex = GM_MUTEX_INITIALIZER;
static pthread_cond_t gProducerPlaying = GM_COND_INITIALIZER;
static int gProducerRunning = 1;
static enum producer_status producer_status = PS_UNLOADED;
static struct input_plugin *ip = NULL;

static pthread_t consumerThread;
static pthread_mutex_t consumer_mutex = GM_MUTEX_INITIALIZER;
static pthread_cond_t gConsumerPlaying = GM_COND_INITIALIZER;
static int gConsumerRunning = 1;
static enum consumer_status consumer_status = CS_STOPPED;
static unsigned long consumer_pos = 0;

/* for replay gain and soft vol
 * usually same as consumer_pos, sometimes more than consumer_pos
 */
static unsigned long scale_pos;
static double replaygain_scale = 1.0;

/* locking {{{ */

#define player_info_priv_lock() gm_mutex_lock(&player_info_mutex)
#define player_info_priv_unlock() gm_mutex_unlock(&player_info_mutex)

#define producer_lock() gm_mutex_lock(&producer_mutex)
#define producer_unlock() gm_mutex_unlock(&producer_mutex)

#define consumer_lock() gm_mutex_lock(&consumer_mutex)
#define consumer_unlock() gm_mutex_unlock(&consumer_mutex)

#define player_lock() \
	do { \
		consumer_lock(); \
		producer_lock(); \
	} while (0)

#define player_unlock() \
	do { \
		producer_unlock(); \
		consumer_unlock(); \
	} while (0)

/* locking }}} */

static void reset_buffer(void)
{
    DEBUG("start");
	buffer_reset();
	consumer_pos = 0;
	scale_pos = 0;
	pthread_cond_broadcast(&gProducerPlaying);
    DEBUG("ok");
}

static void set_buffer_sf(void)
{
	buffer_sf = ip_get_sf(ip);
	ip_get_channel_map(ip, buffer_channel_map);

	/* ip_read converts samples to this format */
	if (sf_get_channels(buffer_sf) <= 2 && sf_get_bits(buffer_sf) <= 16) {
		buffer_sf &= SF_RATE_MASK;
		buffer_sf |= sf_channels(2) | sf_bits(16) | sf_signed(1);
		buffer_sf |= sf_host_endian();
		channel_map_init_stereo(buffer_channel_map);
	}
}

#define SOFT_VOL_SCALE 65536

/* coefficients for volumes 0..99, for 100 65536 is used
 * data copied from alsa-lib src/pcm/pcm_softvol.c
 */
static const unsigned short soft_vol_db[100] = {
	0x0000, 0x0110, 0x011c, 0x012f, 0x013d, 0x0152, 0x0161, 0x0179,
	0x018a, 0x01a5, 0x01c1, 0x01d5, 0x01f5, 0x020b, 0x022e, 0x0247,
	0x026e, 0x028a, 0x02b6, 0x02d5, 0x0306, 0x033a, 0x035f, 0x0399,
	0x03c2, 0x0403, 0x0431, 0x0479, 0x04ac, 0x04fd, 0x0553, 0x058f,
	0x05ef, 0x0633, 0x069e, 0x06ea, 0x0761, 0x07b5, 0x083a, 0x0898,
	0x092c, 0x09cb, 0x0a3a, 0x0aeb, 0x0b67, 0x0c2c, 0x0cb6, 0x0d92,
	0x0e2d, 0x0f21, 0x1027, 0x10de, 0x1202, 0x12cf, 0x1414, 0x14f8,
	0x1662, 0x1761, 0x18f5, 0x1a11, 0x1bd3, 0x1db4, 0x1f06, 0x211d,
	0x2297, 0x24ec, 0x2690, 0x292a, 0x2aff, 0x2de5, 0x30fe, 0x332b,
	0x369f, 0x390d, 0x3ce6, 0x3f9b, 0x43e6, 0x46eb, 0x4bb3, 0x4f11,
	0x5466, 0x5a18, 0x5e19, 0x6472, 0x68ea, 0x6ffd, 0x74f8, 0x7cdc,
	0x826a, 0x8b35, 0x9499, 0x9b35, 0xa5ad, 0xad0b, 0xb8b7, 0xc0ee,
	0xcdf1, 0xd71a, 0xe59c, 0xefd3
};

static inline void scale_sample_int16_t(int16_t *buf, int i, int vol, int swap)
{
	int32_t sample = swap ? (int16_t)swap_uint16(buf[i]) : buf[i];

	if (sample < 0) {
		sample = (sample * vol - SOFT_VOL_SCALE / 2) / SOFT_VOL_SCALE;
		if (sample < INT16_MIN)
			sample = INT16_MIN;
	} else {
		sample = (sample * vol + SOFT_VOL_SCALE / 2) / SOFT_VOL_SCALE;
		if (sample > INT16_MAX)
			sample = INT16_MAX;
	}
	buf[i] = swap ? swap_uint16(sample) : sample;
}

static inline int32_t scale_sample_s24le(int32_t s, int vol)
{
	int64_t sample = s;
	if (sample < 0) {
		sample = (sample * vol - SOFT_VOL_SCALE / 2) / SOFT_VOL_SCALE;
		if (sample < -0x800000)
			sample = -0x800000;
	} else {
		sample = (sample * vol + SOFT_VOL_SCALE / 2) / SOFT_VOL_SCALE;
		if (sample > 0x7fffff)
			sample = 0x7fffff;
	}
	return sample;
}

static inline void scale_sample_int32_t(int32_t *buf, int i, int vol, int swap)
{
	int64_t sample = swap ? (int32_t)swap_uint32(buf[i]) : buf[i];

	if (sample < 0) {
		sample = (sample * vol - SOFT_VOL_SCALE / 2) / SOFT_VOL_SCALE;
		if (sample < INT32_MIN)
			sample = INT32_MIN;
	} else {
		sample = (sample * vol + SOFT_VOL_SCALE / 2) / SOFT_VOL_SCALE;
		if (sample > INT32_MAX)
			sample = INT32_MAX;
	}
	buf[i] = swap ? swap_uint32(sample) : sample;
}

static inline int sf_need_swap(sample_format_t sf)
{
#ifdef WORDS_BIGENDIAN
	return !sf_get_bigendian(sf);
#else
	return sf_get_bigendian(sf);
#endif
}

#define SCALE_SAMPLES(TYPE, buffer, count, l, r, swap)				\
{										\
	const int frames = count / sizeof(TYPE) / 2;				\
	TYPE *buf = (void *) buffer;						\
	int i;									\
	/* avoid underflowing -32768 to 32767 when scale is 65536 */		\
	if (l != SOFT_VOL_SCALE && r != SOFT_VOL_SCALE) {			\
		for (i = 0; i < frames; i++) {					\
			scale_sample_##TYPE(buf, i * 2, l, swap);		\
			scale_sample_##TYPE(buf, i * 2 + 1, r, swap);		\
		}								\
	} else if (l != SOFT_VOL_SCALE) {					\
		for (i = 0; i < frames; i++)					\
			scale_sample_##TYPE(buf, i * 2, l, swap);		\
	} else if (r != SOFT_VOL_SCALE) {					\
		for (i = 0; i < frames; i++)					\
			scale_sample_##TYPE(buf, i * 2 + 1, r, swap);		\
	}									\
}

static inline int32_t read_s24le(const char *buf)
{
	const unsigned char *b = (const unsigned char *) buf;
	return b[0] | (b[1] << 8) | (((const signed char *) buf)[2] << 16);
}

static inline void write_s24le(char *buf, int32_t x)
{
	unsigned char *b = (unsigned char *) buf;
	b[0] = x;
	b[1] = x >> 8;
	b[2] = x >> 16;
}

static void scale_samples_s24le(char *buf, unsigned int count, int l, int r)
{
	int frames = count / 3 / 2;
	if (l != SOFT_VOL_SCALE && r != SOFT_VOL_SCALE) {
		while (frames--) {
			write_s24le(buf, scale_sample_s24le(read_s24le(buf), l));
			buf += 3;
			write_s24le(buf, scale_sample_s24le(read_s24le(buf), r));
			buf += 3;
		}
	} else if (l != SOFT_VOL_SCALE) {
		while (frames--) {
			write_s24le(buf, scale_sample_s24le(read_s24le(buf), l));
			buf += 3 * 2;
		}
	} else if (r != SOFT_VOL_SCALE) {
		buf += 3;
		while (frames--) {
			write_s24le(buf, scale_sample_s24le(read_s24le(buf), r));
			buf += 3 * 2;
		}
	}
}

static void scale_samples(char *buffer, unsigned int *countp)
{
	unsigned int count = *countp;
	int ch, bits, l, r;

	BUG_ON(scale_pos < consumer_pos);

	if (consumer_pos != scale_pos) {
		unsigned int offs = scale_pos - consumer_pos;

		if (offs >= count)
			return;
		buffer += offs;
		count -= offs;
	}
	scale_pos += count;

	if (replaygain_scale == 1.0 && soft_vol_l == 100 && soft_vol_r == 100)
		return;

	ch = sf_get_channels(buffer_sf);
	bits = sf_get_bits(buffer_sf);
	if (ch != 2 || (bits != 16 && bits != 24 && bits != 32))
		return;

	l = SOFT_VOL_SCALE;
	r = SOFT_VOL_SCALE;
	if (soft_vol && soft_vol_l != 100)
		l = soft_vol_db[soft_vol_l];
	if (soft_vol && soft_vol_r != 100)
		r = soft_vol_db[soft_vol_r];

	l *= replaygain_scale;
	r *= replaygain_scale;

	switch (bits) {
	case 16:
		SCALE_SAMPLES(int16_t, buffer, count, l, r, sf_need_swap(buffer_sf));
		break;
	case 24:
		if (likely(!sf_get_bigendian(buffer_sf)))
			scale_samples_s24le(buffer, count, l, r);
		break;
	case 32:
		SCALE_SAMPLES(int32_t, buffer, count, l, r, sf_need_swap(buffer_sf));
		break;
	}
}

static void update_rg_scale(void)
{
	double gain, peak, db, scale, limit;

	replaygain_scale = 1.0;
	if (!player_info_priv.ti || !replaygain)
		return;

	bool avoid_album_gain = replaygain == RG_SMART && (!play_library || shuffle == SHUFFLE_TRACKS || gm_queue_active());
	
	if (replaygain == RG_TRACK || replaygain == RG_TRACK_PREFERRED || avoid_album_gain) {
		gain = player_info_priv.ti->rg_track_gain;
		peak = player_info_priv.ti->rg_track_peak;
	} else {
		gain = player_info_priv.ti->rg_album_gain;
		peak = player_info_priv.ti->rg_album_peak;
	}

	if (isnan(gain)) {
		if (replaygain == RG_TRACK_PREFERRED || avoid_album_gain) {
			gain = player_info_priv.ti->rg_album_gain;
			peak = player_info_priv.ti->rg_album_peak;
		} else if (replaygain == RG_ALBUM_PREFERRED) {
			gain = player_info_priv.ti->rg_track_gain;
			peak = player_info_priv.ti->rg_track_peak;
		}
	}

	if (isnan(gain)) {
		DEBUG ("gain not available\n");
		return;
	}
	if (isnan(peak)) {
		DEBUG ("peak not available, deriving from output gain\n");
		peak = pow(10.0, player_info_priv.ti->output_gain / 20.0);
	}
	if (peak < 0.05) {
		DEBUG ("peak (%g) is too small\n", peak);
		return;
	}

	db = replaygain_preamp + gain;

	scale = pow(10.0, db / 20.0);
	replaygain_scale = scale;
	limit = 1.0 / peak;
	if (replaygain_limit && !isnan(peak)) {
		if (replaygain_scale > limit)
			replaygain_scale = limit;
	}

	DEBUG ("gain = %f, peak = %f, db = %f, scale = %f, limit = %f, replaygain_scale = %f\n",
			gain, peak, db, scale, limit, replaygain_scale);
}

static inline unsigned int buffer_second_size(void)
{
	return sf_get_second_size(buffer_sf);
}

/* updating player status {{{ */

static inline void _file_changed(struct track_info *ti)
{
    DEBUG("START");
	player_info_priv_lock();
	if (player_info_priv.ti) {
        track_info_unref(player_info_priv.ti);
    }

	player_info_priv.ti = ti;
	update_rg_scale();
	player_metadata[0] = 0;
	player_info_priv.file_changed = 1;
	player_info_priv_unlock();
    DEBUG("OK");
}

static inline void file_changed(struct track_info *ti)
{
	if (ti) {
		DEBUG ("file: %s", ti->filename);
	} else {
		DEBUG ("unloaded");
	}
	_file_changed(ti);
}

static inline void metadata_changed(void)
{
    DEBUG ("start");
	KeyValue* comments;
	int rc;

	player_info_priv_lock();
	if (ip_get_metadata(ip)) {
		DEBUG ("metadata changed: %s", ip_get_metadata(ip));
		memcpy(player_metadata, ip_get_metadata(ip), 255 * 16 + 1);
	}

	rc = ip_read_comments(ip, &comments);
	if (!rc) {
		if (player_info_priv.ti->comments)
			key_value_free(player_info_priv.ti->comments);
		track_info_set_comments(player_info_priv.ti, comments);
	}

	player_info_priv.metadata_changed = 1;
	player_info_priv_unlock();
    DEBUG ("OK");
}

static void player_error(const char *msg)
{
	player_info_priv_lock();
	player_info_priv.status = (enum player_status)consumer_status;
	player_info_priv.pos = 0;
	player_info_priv.current_bitrate = -1;
	player_info_priv.buffer_fill = buffer_get_filled_chunks();
	player_info_priv.buffer_size = buffer_nr_chunks;
	player_info_priv.status_changed = 1;

	free(player_info_priv.error_msg);
	player_info_priv.error_msg = xstrdup(msg);
	player_info_priv_unlock();

	ERROR ("ERROR: '%s'", msg);
}

static void CMUS_FORMAT(2, 3) player_ip_error(int rc, const char *format, ...)
{
	char buffer[1024];
	va_list ap;
	char *msg;
	int save = errno;

	va_start(ap, format);
	vsnprintf(buffer, sizeof(buffer), format, ap);
	va_end(ap);

	errno = save;
	msg = ip_get_error_msg(ip, rc, buffer);
	player_error(msg);
	free(msg);
}

static void CMUS_FORMAT(2, 3) player_op_error(int rc, const char *format, ...)
{
	char buffer[1024];
	va_list ap;
	char *msg;
	int save = errno;

	va_start(ap, format);
	vsnprintf(buffer, sizeof(buffer), format, ap);
	va_end(ap);

	errno = save;
	msg = op_get_error_msg(rc, buffer);
	player_error(msg);
	free(msg);
}

/*
 * buffer-fill changed
 */
static void _producer_buffer_fill_update(void)
{
	int fill;

	player_info_priv_lock();
	fill = buffer_get_filled_chunks();
	if (fill != player_info_priv.buffer_fill) {
/* 		DEBUG ("\n"); */
		player_info_priv.buffer_fill = fill;
		player_info_priv.buffer_fill_changed = 1;
	}
	player_info_priv_unlock();
}

/*
 * playing position changed
 */
static void _consumer_position_update(void)
{
	static unsigned int old_pos = -1;
	unsigned int pos = 0;
	long bitrate;

	if (consumer_status == CS_PLAYING || consumer_status == CS_PAUSED)
		pos = consumer_pos / buffer_second_size();
	if (pos != old_pos) {
		old_pos = pos;

		player_info_priv_lock();
		player_info_priv.pos = pos;

		if (show_current_bitrate) {
			bitrate = ip_current_bitrate(ip);
			if (bitrate != -1)
				player_info_priv.current_bitrate = bitrate;
		}
		player_info_priv.position_changed = 1;
		player_info_priv_unlock();
	}
}

/*
 * something big happened (stopped/paused/unpaused...)
 */
static void _player_status_changed(void)
{
	unsigned int pos = 0;

/* 	DEBUG ("\n"); */
	if (consumer_status == CS_PLAYING || consumer_status == CS_PAUSED)
		pos = consumer_pos / buffer_second_size();

	player_info_priv_lock();
	player_info_priv.status = (enum player_status)consumer_status;
	player_info_priv.pos = pos;
	player_info_priv.current_bitrate = -1;
	player_info_priv.buffer_fill = buffer_get_filled_chunks();
	player_info_priv.buffer_size = buffer_nr_chunks;
	player_info_priv.status_changed = 1;
	player_info_priv_unlock();
}

/* updating player status }}} */

static void _prebuffer(void)
{
	int limit_chunks;

	BUG_ON(producer_status != PS_PLAYING);
	if (ip_is_remote(ip)) {
		limit_chunks = buffer_nr_chunks;
	} else {
		int limit_ms, limit_size;

		limit_ms = 250;
		limit_size = limit_ms * buffer_second_size() / 1000;
		limit_chunks = limit_size / CHUNK_SIZE;
		if (limit_chunks < 1)
			limit_chunks = 1;
	}
	while (1) {
		int nr_read, size, filled;
		char *wpos;

		filled = buffer_get_filled_chunks();
/* 		DEBUG ("PREBUF: %2d / %2d\n", filled, limit_chunks); */

		/* not fatal */
		//BUG_ON(filled > limit_chunks);

		if (filled >= limit_chunks)
			break;

		size = buffer_get_wpos(&wpos);
		nr_read = ip_read(ip, wpos, size);
		if (nr_read < 0) {
			if (nr_read == -1 && errno == EAGAIN)
				continue;
			player_ip_error(nr_read, "reading file %s", ip_get_filename(ip));
			/* ip_read sets eof */
			nr_read = 0;
		}
		if (ip_metadata_changed(ip))
			metadata_changed();

		/* buffer_fill with 0 count marks current chunk filled */
		buffer_fill(nr_read);

		_producer_buffer_fill_update();
		if (nr_read == 0) {
			/* EOF */
			break;
		}
	}
}

/* setting producer status {{{ */

static void _producer_status_update(enum producer_status status)
{
    DEBUG ("start");
	producer_status =  status;
	pthread_cond_broadcast(&gProducerPlaying);
    DEBUG ("OK");
}

static void _producer_play(void)
{
	if (producer_status == PS_UNLOADED) {
		struct track_info *ti;

		if ((ti = gm_get_next_track())) {
			int rc;

			ip = ip_new(ti->filename);
			rc = ip_open(ip);
			if (rc) {
				player_ip_error(rc, "opening file `%s'", ti->filename);
				ip_delete(ip);
				track_info_unref(ti);
				file_changed(NULL);
			} else {
				ip_setup(ip);
                DEBUG ("update status: PS_PLAYING");
				_producer_status_update(PS_PLAYING);
				file_changed(ti);
			}
		}
	} else if (producer_status == PS_PLAYING) {
		if (ip_seek(ip, 0.0) == 0) {
			reset_buffer();
		}
	} else if (producer_status == PS_STOPPED) {
		int rc;

		rc = ip_open(ip);
		if (rc) {
			player_ip_error(rc, "opening file `%s'", ip_get_filename(ip));
			ip_delete(ip);
			_producer_status_update(PS_UNLOADED);
		} else {
			ip_setup(ip);
            DEBUG ("update status: PS_PLAYING");
			_producer_status_update(PS_PLAYING);
		}
	} else if (producer_status == PS_PAUSED) {
		_producer_status_update(PS_PLAYING);
	}
}

static void _producer_stop(void)
{
	if (producer_status == PS_PLAYING || producer_status == PS_PAUSED) {
		ip_close(ip);
		_producer_status_update(PS_STOPPED);
		reset_buffer();
	}
}

static void _producer_unload(void)
{
	_producer_stop();
	if (producer_status == PS_STOPPED) {
		ip_delete(ip);
		_producer_status_update(PS_UNLOADED);
	}
}

static void _producer_pause(void)
{
	if (producer_status == PS_PLAYING) {
		_producer_status_update(PS_PAUSED);
	} else if (producer_status == PS_PAUSED) {
		_producer_status_update(PS_PLAYING);
	}
}

static void _producer_set_file(struct track_info *ti)
{
	_producer_unload();
	ip = ip_new(ti->filename);
	_producer_status_update(PS_STOPPED);
	file_changed(ti);
}

/* setting producer status }}} */

/* setting consumer status {{{ */

static void _consumer_status_update(enum consumer_status status)
{
	consumer_status = status;
	pthread_cond_broadcast(&gConsumerPlaying);
}

static void _consumer_play(void)
{
	if (consumer_status == CS_PLAYING) {
		op_drop();
	} else if (consumer_status == CS_STOPPED) {
		int rc;

		set_buffer_sf();
		rc = op_open(buffer_sf, buffer_channel_map);
		if (rc) {
			player_op_error(rc, "opening audio device");
		} else {
			_consumer_status_update(CS_PLAYING);
		}
	} else if (consumer_status == CS_PAUSED) {
		op_unpause();
		_consumer_status_update(CS_PLAYING);
	}
}

static void _consumer_drain_and_stop(void)
{
	if (consumer_status == CS_PLAYING || consumer_status == CS_PAUSED) {
		op_close();
		_consumer_status_update(CS_STOPPED);
	}
}

static void _consumer_stop(void)
{
	if (consumer_status == CS_PLAYING || consumer_status == CS_PAUSED) {
		op_drop();
		op_close();
		_consumer_status_update(CS_STOPPED);
	}
}

static void _consumer_pause(void)
{
	if (consumer_status == CS_PLAYING) {
		op_pause();
		_consumer_status_update(CS_PAUSED);
	} else if (consumer_status == CS_PAUSED) {
		op_unpause();
		_consumer_status_update(CS_PLAYING);
	}
}

/* setting consumer status }}} */

static int change_sf(int drop)
{
	int old_sf = buffer_sf;
	CHANNEL_MAP(old_channel_map);
	channel_map_copy(old_channel_map, buffer_channel_map);

	set_buffer_sf();
	if (buffer_sf != old_sf || !channel_map_equal(buffer_channel_map, old_channel_map, sf_get_channels(buffer_sf))) {
		/* reopen */
		int rc;

		if (drop)
			op_drop();
		op_close();
		rc = op_open(buffer_sf, buffer_channel_map);
		if (rc) {
			player_op_error(rc, "opening audio device");
			_consumer_status_update(CS_STOPPED);
			_producer_stop();
			return rc;
		}
	} else if (consumer_status == CS_PAUSED) {
		op_drop();
		op_unpause();
	}
	_consumer_status_update(CS_PLAYING);
	return 0;
}

static void _consumer_handle_eof(void)
{
	struct track_info *ti;

	if (ip_is_remote(ip)) {
		_producer_stop();
		_consumer_drain_and_stop();
		player_error("lost connection");
		return;
	}

	if (player_info_priv.ti)
		player_info_priv.ti->play_count++;

	if (player_repeat_current) {
		if (player_cont) {
			ip_seek(ip, 0);
			reset_buffer();
		} else {
			_producer_stop();
			_consumer_drain_and_stop();
		}
		_player_status_changed();
		return;
	}

	if ((ti = gm_get_next_track())) {
		_producer_unload();
		ip = ip_new(ti->filename);
		_producer_status_update(PS_STOPPED);
		/* PS_STOPPED, CS_PLAYING */
		if (player_cont && (player_cont_album == 1 || strcmp(player_info_priv.ti->album,ti->album) == 0)) {
			_producer_play();
			if (producer_status == PS_UNLOADED) {
				_consumer_stop();
				track_info_unref(ti);
				file_changed(NULL);
			} else {
				/* PS_PLAYING */
				file_changed(ti);
				if (!change_sf(0))
					_prebuffer();
			}
		} else {
			_consumer_drain_and_stop();
			file_changed(ti);
		}
	} else {
		_producer_unload();
		_consumer_drain_and_stop();
		file_changed(NULL);
	}
	_player_status_changed();
}

static void *consumer_loop(void *arg)
{
	while (1) {
		int rc, space;
		int size;
		char *rpos;

		consumer_lock();
		if (!gConsumerRunning) {
            break;
        }

		if (consumer_status == CS_PAUSED || consumer_status == CS_STOPPED) {
			pthread_cond_wait(&gConsumerPlaying, &consumer_mutex);
			consumer_unlock();
			continue;
		}
		space = op_buffer_space();
		if (space < 0) {
			DEBUG ("op_buffer_space returned %d %s\n", space, space == -1 ? strerror(errno) : "");

			/* try to reopen */
			op_close();
			_consumer_status_update(CS_STOPPED);
			_consumer_play();

			consumer_unlock();
			continue;
		}
        //DEBUG ("BS: %6d %3d", space, space * 1000 / (44100 * 2 * 2));

		while (1) {
			if (space == 0) {
				_consumer_position_update();
				consumer_unlock();
				ms_sleep(25);
				break;
			}
			size = buffer_get_rpos(&rpos);
			if (size == 0) {
				producer_lock();
				if (producer_status != PS_PLAYING) {
					producer_unlock();
					consumer_unlock();
					break;
				}
				/* must recheck rpos */
				size = buffer_get_rpos(&rpos);
				if (size == 0) {
					/* OK. now it's safe to check if we are at EOF */
					if (ip_eof(ip)) {
						/* EOF */
						_consumer_handle_eof();
						producer_unlock();
						consumer_unlock();
						break;
					} else {
						/* possible underrun */
						producer_unlock();
						_consumer_position_update();
						consumer_unlock();
/* 						DEBUG ("possible underrun\n"); */
						ms_sleep(10);
						break;
					}
				}

				/* player_buffer and ip.eof were inconsistent */
				producer_unlock();
			}
			if (size > space)
				size = space;
			if (soft_vol || replaygain)
				scale_samples(rpos, (unsigned int *)&size);
			rc = op_write(rpos, size);
			if (rc < 0) {
				DEBUG ("op_write returned %d %s", rc, rc == -1 ? strerror(errno) : "");

				/* try to reopen */
				op_close();
				_consumer_status_update(CS_STOPPED);
				_consumer_play();

				consumer_unlock();
				break;
			}
			buffer_consume(rc);
			consumer_pos += rc;
			space -= rc;
		}
	}
	_consumer_stop();
	consumer_unlock();

	return NULL;
}

static void *producer_loop(void *arg)
{
	while (1) {
		/* number of chunks to fill
		 * too big   => seeking is slow
		 * too small => underruns?
		 */
		const int chunks = 1;
		int size, nr_read, i;
		char *wpos;

		producer_lock();
		if (!gProducerRunning) {
            break;
        }

		if (producer_status == PS_UNLOADED ||
		    producer_status == PS_PAUSED ||
		    producer_status == PS_STOPPED || ip_eof(ip)) {
			pthread_cond_wait(&gProducerPlaying, &producer_mutex);
			producer_unlock();
			continue;
		}
		for (i = 0; ; i++) {
			size = buffer_get_wpos(&wpos);
			if (size == 0) {
				/* buffer is full */
				producer_unlock();
				ms_sleep(50);
				break;
			}
			nr_read = ip_read(ip, wpos, size);
			if (nr_read < 0) {
				if (nr_read != -1 || errno != EAGAIN) {
					player_ip_error(nr_read, "reading file %s", ip_get_filename(ip));
					/* ip_read sets eof */
					nr_read = 0;
				} else {
					producer_unlock();
					ms_sleep(50);
					break;
				}
			}
			if (ip_metadata_changed(ip)) {
                metadata_changed();
            }

			/* buffer_fill with 0 count marks current chunk filled */
			buffer_fill(nr_read);
			if (nr_read == 0) {
				/* consumer handles EOF */
				producer_unlock();
				ms_sleep(50);
				break;
			}
			if (i == chunks) {
				producer_unlock();
				/* don't sleep! */
				break;
			}
		}
		_producer_buffer_fill_update();
	}
	_producer_unload();
	producer_unlock();
	return NULL;
}

void player_init(void)
{
	pthread_attr_t              attr;
	pthread_attr_t*             attrP = NULL;

	/*  1 s is 176400 B (0.168 MB)
	 * 10 s is 1.68 MB
	 */
	buffer_nr_chunks = 10 * 44100 * 16 / 8 * 2 / CHUNK_SIZE;
	buffer_init();

	int rc = pthread_attr_init(&attr);
	rc = pthread_attr_setschedpolicy(&attr, SCHED_RR);
	if (rc) {
		WARNING ("could not set real-time scheduling priority: %s", strerror(rc));
	} else {
		struct sched_param param;
		DEBUG ("using real-time scheduling");
		param.sched_priority = sched_get_priority_max(SCHED_RR);
		DEBUG ("setting priority to %d", param.sched_priority);
		rc = pthread_attr_setschedparam(&attr, &param);
		BUG_ON(rc);
		attrP = &attr;
	}


	rc = pthread_create(&gProducerThread, NULL, producer_loop, NULL);
	rc = pthread_create(&consumerThread, attrP, consumer_loop, NULL);
	if (rc && attrP) {
		DEBUG ("could not create thread using real-time scheduling: %s", strerror(rc));
		rc = pthread_create(&consumerThread, NULL, consumer_loop, NULL);
	}


	/* update player_info_priv.cont etc. */
	player_lock();
	_player_status_changed();
	player_unlock();
}

void player_exit(void)
{
	int rc;

	player_lock();
	gConsumerRunning = 0;
	pthread_cond_broadcast(&gConsumerPlaying);
	gProducerRunning = 0;
	pthread_cond_broadcast(&gProducerPlaying);
	player_unlock();

	rc = pthread_join(consumerThread, NULL);
	rc = pthread_join(gProducerThread, NULL);
	buffer_free();
}

void player_stop(void)
{
	player_lock();
	_consumer_stop();
	_producer_stop();
	_player_status_changed();
	player_unlock();
}

void player_play(void)
{
	int prebuffer;

	player_lock();
	if (producer_status == PS_PLAYING && ip_is_remote(ip)) {
		/* seeking not allowed */
		player_unlock();
		return;
	}
	prebuffer = consumer_status == CS_STOPPED;
	_producer_play();
	if (producer_status == PS_PLAYING) {
		_consumer_play();
		if (consumer_status != CS_PLAYING)
			_producer_stop();
	} else {
		_consumer_stop();
	}
	_player_status_changed();
	if (consumer_status == CS_PLAYING && prebuffer)
		_prebuffer();
	player_unlock();
}

void player_pause(void)
{
	if (ip && ip_is_remote(ip) && consumer_status == CS_PLAYING) {
		/* pausing not allowed */
		player_stop();
		return;
	}
	player_lock();

	if (consumer_status == CS_STOPPED) {
		_producer_play();
		if (producer_status == PS_PLAYING) {
			_consumer_play();
			if (consumer_status != CS_PLAYING)
				_producer_stop();
		}
		_player_status_changed();
		if (consumer_status == CS_PLAYING)
			_prebuffer();
		player_unlock();
		return;
	}

	_producer_pause();
	_consumer_pause();
	_player_status_changed();
	player_unlock();
}

void player_pause_playback(void)
{
	if (consumer_status == CS_PLAYING)
		player_pause();
}

void player_set_file(struct track_info *ti)
{
	player_lock();
	_producer_set_file(ti);
	if (producer_status == PS_UNLOADED) {
		_consumer_stop();
		goto out;
	}

	/* PS_STOPPED */
	if (consumer_status == CS_PLAYING || consumer_status == CS_PAUSED) {
		op_drop();
		_producer_play();
		if (producer_status == PS_UNLOADED) {
			_consumer_stop();
			goto out;
		}
		change_sf(1);
	}
out:
	_player_status_changed();
	if (producer_status == PS_PLAYING)
		_prebuffer();
	player_unlock();
}

void player_play_file(struct track_info *ti)
{
	player_lock();
	_producer_set_file(ti);
	if (producer_status == PS_UNLOADED) {
		_consumer_stop();
		goto out;
	}

	/* PS_STOPPED */
	_producer_play();

	/* PS_UNLOADED,PS_PLAYING */
	if (producer_status == PS_UNLOADED) {
		_consumer_stop();
		goto out;
	}

	/* PS_PLAYING */
	if (consumer_status == CS_STOPPED) {
		_consumer_play();
		if (consumer_status == CS_STOPPED)
			_producer_stop();
	} else {
		op_drop();
		change_sf(1);
	}
out:
	_player_status_changed();
	if (producer_status == PS_PLAYING)
		_prebuffer();
	player_unlock();
}

void player_file_changed(struct track_info *ti)
{
	_file_changed(ti);
}

void player_seek(double offset, int relative, int start_playing)
{
	int stopped = 0;
	player_lock();
	if (consumer_status == CS_STOPPED) {
		stopped = 1;
		_producer_play();
		if (producer_status == PS_PLAYING) {
			_consumer_play();
			if (consumer_status != CS_PLAYING) {
				_producer_stop();
				player_unlock();
				return;
			}
            else {
                _player_status_changed();
            }
		}
	}
	if (consumer_status == CS_PLAYING || consumer_status == CS_PAUSED) {
		double pos, duration, new_pos;
		int rc;

		pos = (double)consumer_pos / (double)buffer_second_size();
		duration = ip_duration(ip);
		if (duration < 0) {
			/* can't seek */
			LOG_DEBUG ("can't seek")
			player_unlock();
			return;
		}
		if (relative) {
			new_pos = pos + offset;
			if (new_pos < 0.0)
				new_pos = 0.0;
			if (offset > 0.0) {
				/* seeking forward */
				if (new_pos > duration) {
					player_unlock();
					gm_next();
					return;
				}
				if (new_pos < 0.0)
					new_pos = 0.0;
				if (new_pos < pos - 0.5) {
					/* must seek at least 0.5s */
					LOG_DEBUG ("must seek at least 0.5s");
					player_unlock();
					return;
				}
			}
		} else {
			new_pos = offset;
			if (new_pos < 0.0) {
				LOG_DEBUG ("seek offset negative");
				player_unlock();
				return;
			}
			if (new_pos > duration - 5.0) {
				new_pos = duration - 5.0;
				if (new_pos < 0.0)
					new_pos = 0.0;
			}
		}
/* 		DEBUG ("seeking %g/%g (%g from eof)\n", new_pos, duration, duration - new_pos); */
		rc = ip_seek(ip, new_pos);
		if (rc == 0) {
			LOG_DEBUG ("doing op_drop after seek");
			op_drop();
			reset_buffer();
			consumer_pos = new_pos * buffer_second_size();
			scale_pos = consumer_pos;
			_consumer_position_update();
			if (stopped && !start_playing) {
				_producer_pause();
				_consumer_pause();
				_player_status_changed();
			}
		} else {
			player_ip_error(rc, "seeking in file %s", ip_get_filename(ip));
			LOG_DEBUG ("error: ip_seek returned %d", rc);
		}
	}
	mpris_seeked();
	player_unlock();
}

/*
 * change output plugin without stopping playback
 */
void player_set_op(const char *name)
{
	int rc = 0;

	player_lock();

	/* drop needed because close drains the buffer */
	if (consumer_status == CS_PAUSED)
		op_drop();

	if (consumer_status == CS_PLAYING || consumer_status == CS_PAUSED)
		op_close();

//	if (name) {
//		LOG_DEBUG ("setting op to '%s'", name);
//		rc = op_select(name);
//	} else {
//		/* first initialized plugin */
//		LOG_DEBUG ("selecting first initialized op");
//		rc = op_select_any();
//	}
	if (rc) {
		_consumer_status_update(CS_STOPPED);

		_producer_stop();
		if (name)
			player_op_error(rc, "selecting output plugin '%s'", name);
		else
			player_op_error(rc, "selecting any output plugin");
		player_unlock();
		return;
	}

	if (consumer_status == CS_PLAYING || consumer_status == CS_PAUSED) {
		set_buffer_sf();
		rc = op_open(buffer_sf, buffer_channel_map);
		if (rc) {
			_consumer_status_update(CS_STOPPED);
			_producer_stop();
			player_op_error(rc, "opening audio device");
			player_unlock();
			return;
		}
		if (consumer_status == CS_PAUSED)
			op_pause();
	}

	player_unlock();
}

void player_set_buffer_chunks(unsigned int nr_chunks)
{
	player_lock();
	_producer_stop();
	_consumer_stop();

	buffer_nr_chunks = nr_chunks;
	buffer_init();

	_player_status_changed();
	player_unlock();
}

int player_get_buffer_chunks(void)
{
	return buffer_nr_chunks;
}

void player_set_soft_volume(int l, int r)
{
	consumer_lock();
	soft_vol_l = l;
	soft_vol_r = r;
	consumer_unlock();
}

void player_set_soft_vol(int soft)
{
	consumer_lock();
	/* don't mess with scale_pos if soft_vol or replaygain is already enabled */
	if (!soft_vol && !replaygain)
		scale_pos = consumer_pos;
	soft_vol = soft;
	consumer_unlock();
}

static int calc_vol(int val, int old, int max_vol, unsigned int flags)
{
	if (flags & VF_RELATIVE) {
		if (flags & VF_PERCENTAGE)
			val = scale_from_percentage(val, max_vol);
		val += old;
	} else if (flags & VF_PERCENTAGE) {
		val = scale_from_percentage(val, max_vol);
	}
	return clamp(val, 0, max_vol);
}

int player_set_vol(int l, int lf, int r, int rf)
{
	int rc = OUTPUT_ERROR_SUCCESS;
	if (soft_vol) {
		l = calc_vol(l, soft_vol_l, 100, lf);
		r = calc_vol(r, soft_vol_r, 100, rf);
		player_set_soft_volume(l, r);
	} else {
		mixer_read_volume();
		l = calc_vol(l, volume_l, volume_max, lf);
		r = calc_vol(r, volume_r, volume_max, rf);
		rc = mixer_set_volume(l, r);
		mixer_read_volume();
	}
	return rc;
}

void player_set_rg(enum replaygain rg)
{
	player_lock();
	/* don't mess with scale_pos if soft_vol or replaygain is already enabled */
	if (!soft_vol && !replaygain)
		scale_pos = consumer_pos;
	replaygain = rg;

	player_info_priv_lock();
	update_rg_scale();
	player_info_priv_unlock();

	player_unlock();
}

void player_set_rg_limit(int limit)
{
	player_lock();
	replaygain_limit = limit;

	player_info_priv_lock();
	update_rg_scale();
	player_info_priv_unlock();

	player_unlock();
}

void player_set_rg_preamp(double db)
{
	player_lock();
	replaygain_preamp = db;

	player_info_priv_lock();
	update_rg_scale();
	player_info_priv_unlock();

	player_unlock();
}

void player_info_snapshot(void)
{
	player_info_priv_lock();

	free(player_info.error_msg);
	if (player_info.ti)
		track_info_unref(player_info.ti);
	memcpy(&player_info, &player_info_priv, sizeof(player_info));
	if (player_info.ti)
		track_info_ref(player_info.ti);

	player_info_priv.file_changed = 0;
	player_info_priv.metadata_changed = 0;
	player_info_priv.status_changed = 0;
	player_info_priv.position_changed = 0;
	player_info_priv.buffer_fill_changed = 0;
	player_info_priv.error_msg = NULL;

	player_info_priv_unlock();
}

void player_metadata_lock(void)
{
	gm_mutex_lock(&player_info_mutex);
}

void player_metadata_unlock(void)
{
	gm_mutex_unlock(&player_info_mutex);
}
