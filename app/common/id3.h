//
// Created by dingjing on 2/13/23.
//

#ifndef GRACEFUL_MUSIC_ID_3_H
#define GRACEFUL_MUSIC_ID_3_H

#include <stdint.h>

/* flags for id3_read_tags */
#define ID3_V1	(1 << 0)
#define ID3_V2	(1 << 1)

typedef struct _Id3Tag                  Id3Tag;

enum Id3Key
{
    ID3_ARTIST,
    ID3_ALBUM,
    ID3_TITLE,
    ID3_DATE,
    ID3_ORIGINAL_DATE,
    ID3_GENRE,
    ID3_DISC,
    ID3_TRACK,
    ID3_ALBUM_ARTIST,
    ID3_ARTIST_SORT,
    ID3_ALBUM_ARTIST_SORT,
    ID3_ALBUM_SORT,
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

struct _Id3Tag
{
    char            v1[128];
    char            *v2[NUM_ID3_KEYS];

    unsigned int    hasV1 : 1;
    unsigned int    hasV2 : 1;
};

extern const char* const gId3KeyNames[NUM_ID3_KEYS];

int id3_tag_size(const char *buf, int buf_size);

void id3_init(Id3Tag* id3);
void id3_free(Id3Tag* id3);

int id3_read_tags(Id3Tag* id3, int fd, unsigned int flags);
char* id3_get_comment(Id3Tag* id3, enum Id3Key key);

char const *id3_get_genre(uint16_t id);

#endif //GRACEFUL_MUSIC_ID_3_H
