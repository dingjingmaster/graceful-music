#ifndef CMUS_ID3_H
#define CMUS_ID3_H

#include <stdint.h>

/* flags for id3_read_tags */
#define ID3_V1	(1 << 0)
#define ID3_V2	(1 << 1)

enum id3_key {
	ID3_ARTIST,
	ID3_ALBUM,
	ID3_TITLE,
	ID3_DATE,
	ID3_ORIGINALDATE,
	ID3_GENRE,
	ID3_DISC,
	ID3_TRACK,
	ID3_ALBUMARTIST,
	ID3_ARTISTSORT,
	ID3_ALBUMARTISTSORT,
	ID3_ALBUMSORT,
	ID3_COMPILATION,
	ID3_RG_TRACK_GAIN,
	ID3_RG_TRACK_PEAK,
	ID3_RG_ALBUM_GAIN,
	ID3_RG_ALBUM_PEAK,
	ID3_COMPOSER,
	ID3_CONDUCTOR,
	ID3_LYRICIST,
	ID3_REMIXER,
	ID3_LABEL,
	ID3_PUBLISHER,
	ID3_SUBTITLE,
	ID3_COMMENT,
	ID3_MUSICBRAINZ_TRACKID,
	ID3_MEDIA,
	ID3_BPM,

	NUM_ID3_KEYS
};

struct id3tag {
	char v1[128];
	char *v2[NUM_ID3_KEYS];

	unsigned int has_v1 : 1;
	unsigned int has_v2 : 1;
};

extern const char * const id3_key_names[NUM_ID3_KEYS];

int id3_tag_size(const char *buf, int buf_size);

void id3_init(struct id3tag *id3);
void id3_free(struct id3tag *id3);

int id3_read_tags(struct id3tag *id3, int fd, unsigned int flags);
char *id3_get_comment(struct id3tag *id3, enum id3_key key);

char const *id3_get_genre(uint16_t id);

#endif
