#ifndef CMUS_MPRIS_H
#define CMUS_MPRIS_H

#define CONFIG_MPRIS
#ifdef CONFIG_MPRIS

extern int mpris_fd;
void mpris_init(void);
void mpris_process(void);
void mpris_free(void);
void mpris_playback_status_changed(void);
void mpris_loop_status_changed(void);
void mpris_shuffle_changed(void);
void mpris_volume_changed(void);
void mpris_metadata_changed(void);
void mpris_seeked(void);

#else

#define mpris_fd (-1)
#define mpris_init() { }
#define mpris_process() { }
#define mpris_free() { }
#define mpris_playback_status_changed() { }
#define mpris_loop_status_changed() { }
#define mpris_shuffle_changed() { }
#define mpris_volume_changed() { }
#define mpris_metadata_changed() { }
#define mpris_seeked() { }

#endif

#endif
