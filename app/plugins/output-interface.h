//
// Created by dingjing on 2/10/23.
//

#ifndef GRACEFUL_MUSIC_OUTPUT_INTERFACE_H
#define GRACEFUL_MUSIC_OUTPUT_INTERFACE_H
#include "sf.h"
#include "../global.h"
#include "channel-map.h"

#ifdef __GNUC__
#include <fcntl.h>
#endif

#define OUTPUT_ABI_VERSION                  2

typedef struct _OutputPlugin                OutputPlugin;
typedef struct _OutputPluginOps             OutputPluginOps;
typedef struct _OutputPluginOpt             OutputPluginOpt;

enum
{
    OUTPUT_ERROR_SUCCESS,                   // no error
    OUTPUT_ERROR_ERRNO,                     // system error (error code in errno)
    OUTPUT_ERROR_NO_PLUGIN,                 // no such plugin
    OUTPUT_ERROR_NOT_INITIALIZED,           // plugin not initialized
    OUTPUT_ERROR_NOT_SUPPORTED,             // function not supported
    OUTPUT_ERROR_NOT_OPEN,                  // mixer not open
    OUTPUT_ERROR_SAMPLE_FORMAT,             // plugin does not support the sample format
    OUTPUT_ERROR_NOT_OPTION,                // plugin does not have this option
    OUTPUT_ERROR_INTERNAL                   //
};

struct _OutputPluginOps
{
    int (*Init)         (void);
    int (*Exit)         (void);
    int (*Open)         (SampleFormat sf, const ChannelPosition* channelMap);
    int (*Close)        (void);
    int (*Drop)         (void);
    int (*Write)        (const char *buffer, int count);
    int (*BufferSpace)  (void);

    /* these can be NULL */
    int (*Pause)        (void);
    int (*Unpause)      (void);

};

#define OPT(prefix, name) { #name, prefix ## _set_ ## name, prefix ## _get_ ## name }

struct _OutputPluginOpt
{
    const char*         name;
    int (*Get)          (char** val);
    int (*Set)          (const char* val);
};

/* symbols exported by plugin */
extern const OutputPluginOps            gOpPcmOps;
extern const OutputPluginOpt            gOpPcmOptions[];
extern const int                        gOpPriority;
extern const unsigned                   gOpAbiVersion;



#endif //GRACEFUL_MUSIC_OUTPUT_INTERFACE_H
