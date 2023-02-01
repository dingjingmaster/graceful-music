//
// Created by dingjing on 2/1/23.
//

#ifndef GRACEFUL_MUSIC_TRACK_INFO_H
#define GRACEFUL_MUSIC_TRACK_INFO_H

#include <time.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "key-value.h"

#define SORT_INVALID                        ((SortKey) (-1))
#define SORT_ARTIST                         (offsetof(TrackInfo, collKeyArtist))
#define SORT_ALBUM                          (offsetof(TrackInfo, collKeyAlbum))
#define SORT_TITLE                          (offsetof(TrackInfo, collKeyTittle))
#define SORT_TRACK_NUMBER                   (offsetof(TrackInfo, trackNumber))
#define SORT_DISC_NUMBER                    (offsetof(TrackInfo, discNumber))
#define SORT_DATE          	                (offsetof(TrackInfo, date))
#define SORT_ORIGINAL_DATE  	            (offsetof(TrackInfo, originalDate))
#define SORT_RG_TRACK_GAIN 	                (offsetof(TrackInfo, rgTrackGain))
#define SORT_RG_TRACK_PEAK 	                (offsetof(TrackInfo, rgTrackPeak))
#define SORT_RG_ALBUM_GAIN 	                (offsetof(TrackInfo, rgAlbumGain))
#define SORT_RG_ALBUM_PEAK 	                (offsetof(TrackInfo, rgAlbumPeak))
#define SORT_GENRE         	                (offsetof(TrackInfo, collKeyGenre))
#define SORT_COMMENT       	                (offsetof(TrackInfo, collKeyComment))
#define SORT_ALBUM_ARTIST   	            (offsetof(TrackInfo, collKeyAlbumArtist))
#define SORT_PLAY_COUNT   	                (offsetof(TrackInfo, playCount))
#define SORT_FILE_NAME      	            (offsetof(TrackInfo, fileName))
#define SORT_FILE_MTIME     	            (offsetof(TrackInfo, mTime))
#define SORT_BITRATE       	                (offsetof(TrackInfo, bitrate))
#define SORT_CODEC         	                (offsetof(TrackInfo, codec))
#define SORT_CODEC_PROFILE 	                (offsetof(TrackInfo, codecProfile))
#define SORT_MEDIA		                    (offsetof(TrackInfo, media))
#define SORT_BPM		                    (offsetof(TrackInfo, bpm))
#define REV_SORT__START		                (sizeof(TrackInfo))

#define REV_SORT_ARTIST		                (REV_SORT__START + offsetof(TrackInfo, collKeyArtist))
#define REV_SORT_ALBUM                      (REV_SORT__START + offsetof(TrackInfo, collKeyAlbum))
#define REV_SORT_TITLE                      (REV_SORT__START + offsetof(TrackInfo, collKeyTitle))
#define REV_SORT_PLAY_COUNT   	            (REV_SORT__START + offsetof(TrackInfo, playCount))
#define REV_SORT_TRACK_NUMBER               (REV_SORT__START + offsetof(TrackInfo, trackNumber))
#define REV_SORT_DISC_NUMBER                (REV_SORT__START + offsetof(TrackInfo, discNumber))
#define REV_SORT_DATE                       (REV_SORT__START + offsetof(TrackInfo, date))
#define REV_SORT_ORIGINAL_DATE              (REV_SORT__START + offsetof(TrackInfo, originalDate))
#define REV_SORT_RG_TRACK_GAIN              (REV_SORT__START + offsetof(TrackInfo, rgTrackGain))
#define REV_SORT_RG_TRACK_PEAK              (REV_SORT__START + offsetof(TrackInfo, rgTrackPeak))
#define REV_SORT_RG_ALBUM_GAIN              (REV_SORT__START + offsetof(TrackInfo, rgAlbumGain))
#define REV_SORT_RG_ALBUM_PEAK              (REV_SORT__START + offsetof(TrackInfo, rgAlbumPeak))
#define REV_SORT_GENRE                      (REV_SORT__START + offsetof(TrackInfo, collKeyGenre))
#define REV_SORT_COMMENT                    (REV_SORT__START + offsetof(TrackInfo, collKeyComment))
#define REV_SORT_ALBUM_ARTIST               (REV_SORT__START + offsetof(TrackInfo, collKeyAlbumArtist))
#define REV_SORT_FILE_NAME                  (REV_SORT__START + offsetof(TrackInfo, fileName))
#define REV_SORT_FILE_MTIME                 (REV_SORT__START + offsetof(TrackInfo, mTime))
#define REV_SORT_BITRATE                    (REV_SORT__START + offsetof(TrackInfo, bitrate))
#define REV_SORT_CODEC                      (REV_SORT__START + offsetof(TrackInfo, codec))
#define REV_SORT_CODEC_PROFILE              (REV_SORT__START + offsetof(TrackInfo, codecProfile))
#define REV_SORT_MEDIA                      (REV_SORT__START + offsetof(TrackInfo, media))
#define REV_SORT_BPM                        (REV_SORT__START + offsetof(TrackInfo, bpm))

#define TI_MATCH_ARTIST                     (1 << 0)
#define TI_MATCH_ALBUM                      (1 << 1)
#define TI_MATCH_TITLE                      (1 << 2)
#define TI_MATCH_ALBUM_ARTIST               (1 << 3)
#define TI_MATCH_ALL                        (~0)

typedef size_t                              SortKey;
typedef struct _TrackInfo                   TrackInfo;

struct _TrackInfo
{
    uint64_t                                uid;
    KeyValue*                               comments;
    TrackInfo*                              next;

    time_t                                  mTime;
    int                                     duration;
    long                                    bitrate;
    char*                                   codec;
    char*                                   codecProfile;
    char*                                   fileName;

    int                                     trackNumber;
    int                                     discNumber;
    int                                     date;
    int                                     originalDate;

    double                                  rgTrackGain;
    double                                  rgTrackPeak;
    double                                  rgAlbumGain;
    double                                  rgAlbumPeak;
    double                                  outputGain;

    const char*                             artList;
    const char*                             album;
    const char*                             title;
    const char*                             genre;
    const char*                             comment;
    const char*                             albumArtist;
    const char*                             artistSort;
    const char*                             albumSort;
    const char*                             media;

    char*                                   collKeyArtist;
    char*                                   collKeyAlbum;
    char*                                   collKeyTitle;
    char*                                   collKeyGenre;
    char*                                   collKeyComment;
    char*                                   collKeyAlbumArtist;

    unsigned int                            playCount;

    int                                     isVaCompilation : 1;
    int                                     bpm;
};

TrackInfo*  track_info_new              (const char* fileName);
void        track_info_set_comments     (TrackInfo* ti, KeyValue* comments);
void        track_info_ref              (TrackInfo* ti);
void        track_info_unref            (TrackInfo* ti);
bool        track_info_unique_ref       (TrackInfo* ti);

/**
 * @return
 *      1 if @ti has any of the following tags: artist, album, title
 *      0 otherwise
 */
int         track_info_has_tag          (const TrackInfo* ti);

/**
 * @flags  fields to search in (TI_MATCH_*)
 *
 * @return
 *      1 if all words in @text are found to match defined fields (@flags) in @ti
 *      0 otherwise
 */
int         track_info_matches          (const TrackInfo* ti, const char* text, unsigned int flags);

/**
 * @flags            fields to search in (TI_MATCH_*)
 * @exclude_flags    fields which must not match (TI_MATCH_*)
 * @match_all_words  if true, all words must be found in @ti
 *
 * @return
        1 if all/any words in @text are found to match defined fields (@flags) in @ti
 *      0 otherwise
 */
int         track_info_matches_full     (const TrackInfo* ti, const char* text, unsigned int flags, unsigned int excludeFlags, int matchAllWords);

int         track_info_cmp              (const TrackInfo* a, const TrackInfo *b, const SortKey* keys);

SortKey*    parse_sort_keys             (const char* value);
const char* sort_key_to_str             (SortKey key);
void        sort_keys_to_str            (const SortKey* keys, char* buf, size_t bufSize);

#endif //GRACEFUL_MUSIC_TRACK_INFO_H
