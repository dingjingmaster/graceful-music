#ifndef CMUS_CUE_UTILS_H
#define CMUS_CUE_UTILS_H

#include <stdio.h>

char *associated_cue(const char *filename);
int cue_get_ntracks(const char *filename);
char *construct_cue_url(const char *cue_filename, int track_n);


#endif
