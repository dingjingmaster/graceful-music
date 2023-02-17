//
// Created by dingjing on 23-2-17.
//

#ifndef GRACEFUL_MUSIC_GRACEFUL_MUSIC_H
#define GRACEFUL_MUSIC_GRACEFUL_MUSIC_H
#include "track_info.h"

typedef enum _FileType              FileType;

enum _FileType
{
    FILE_TYPE_INVALID,
    FILE_TYPE_URL,
    FILE_TYPE_PL,
    FILE_TYPE_DIR,
    FILE_TYPE_FILE,
    FILE_TYPE_CDDA
};

typedef int (*track_info_cb)(void *data, struct track_info *ti);

/* lib_for_each, lib_for_each_filtered, pl_for_each, play_queue_for_each */
typedef int (*for_each_ti_cb)(track_info_cb cb, void *data, void *opaque);

/* lib_for_each_sel, pl_for_each_sel, play_queue_for_each_sel */
typedef int (*for_each_sel_ti_cb)(track_info_cb cb, void *data, int reverse, int advance);

/* lib_add_track, pl_add_track, play_queue_append, play_queue_prepend */
typedef void (*add_ti_cb)(struct track_info *, void *opaque);

/* cmus_save, cmus_save_ext */
typedef int (*save_ti_cb)(for_each_ti_cb for_each_ti, const char *filename, void *opaque);


int gm_init(void);
void gm_exit(void);
void gm_play_file(const char *filename);

/* detect file type, returns absolute path or url in @ret */
FileType gm_detect_ft (const char *name, char **ret);

/* add to library, playlist or queue view
 *
 * @add   callback that does the actual adding
 * @name  playlist, directory, file, URL
 * @ft    detected FILE_TYPE_*
 * @jt    JOB_TYPE_{LIB,PL,QUEUE}
 *
 * returns immediately, actual work is done in the worker thread.
 */
void gm_add (add_ti_cb, const char *name, FileType ft, int jt, int force, void* opaque);

int gm_save (for_each_ti_cb for_each_ti, const char* filename, void* opaque);
int gm_save_ext (for_each_ti_cb for_each_ti, const char* filename, void* opaque);

void gm_update_lib (void);
void gm_update_cache (int force);
void gm_update_tis (struct track_info** tis, int nr, int force);

int gm_is_playlist (const char* filename);
int gm_is_playable (const char* filename);
int gm_is_supported (const char* filename);

int gm_playlist_for_each(const char *buf, int size, int reverse, int (*cb)(void *data, const char *line), void *data);

void gm_next (void);
void gm_prev (void);
void gm_next_album (void);
void gm_prev_album (void);

extern int gNextTrackRequestFd;
struct track_info* gm_get_next_track(void);
void gm_provide_next_track(void);
void gm_track_request_init(void);

int gm_can_raise_vte(void);
void gm_raise_vte(void);

bool gm_queue_active(void);

#endif //GRACEFUL_MUSIC_GRACEFUL_MUSIC_H
