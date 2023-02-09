#ifndef CMUS_PCM_H
#define CMUS_PCM_H

typedef void (*pcm_conv_func)(void *dst, const void *src, int count);
typedef void (*pcm_conv_in_place_func)(void *buf, int count);

extern pcm_conv_func pcm_conv[8];
extern pcm_conv_in_place_func pcm_conv_in_place[8];

#endif
