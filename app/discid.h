#ifndef CMUS_DISCID_H
#define CMUS_DISCID_H

char *get_default_cdda_device(void);
int parse_cdda_url(const char *url, char **disc_id, int *start_track, int *end_track);
char *gen_cdda_url(const char *disc_id, int start_track, int end_track);
char *complete_cdda_url(const char *device, const char *url);
int get_disc_id(const char *device, char **disc_id, int *num_tracks);

#endif
