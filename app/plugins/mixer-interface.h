//
// Created by dingjing on 2/10/23.
//

#ifndef GRACEFUL_MUSIC_MIXER_INTERFACE_H
#define GRACEFUL_MUSIC_MIXER_INTERFACE_H

#ifndef __GNUC__
#include <fcntl.h>
#endif

#define NR_MIXER_FDS                    4

typedef struct _MixerPluginOps          MixerPluginOps;
typedef struct _MixerPluginOpt          MixerPluginOpt;

enum
{
    MIXER_FDS_VOLUME,                   // volume changes
    MIXER_FDS_OUTPUT                    // output changes
};

struct _MixerPluginOps
{
    int (*Init)     (void);
    int (*Exit)     (void);
    int (*Open)     (int* volumeMax);
    int (*Close)    (void);
    union {
        int (*abi1) (int* fds);         // MIXER_FDS_VOLUME
        int (*abi2) (int what, int* fds);
    } getFds;
    int (*SetVolume)(int l, int r);
    int (*GetVolume)(int* l, int* r);
};

struct _MixerPluginOpt
{
    const char*     name;
    int (*Get)      (char** val);
    int (*Set)      (const char* val);
};

/* symbols exported by plugin */
extern const MixerPluginOps gOutputMixerOps;
extern const MixerPluginOpt gOutputMixerOptions[];

#endif //GRACEFUL_MUSIC_MIXER_INTERFACE_H
