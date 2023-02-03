//
// Created by dingjing on 2/2/23.
//

#ifndef GRACEFUL_MUSIC_INPUT_BASE_H
#define GRACEFUL_MUSIC_INPUT_BASE_H

#include "global.h"
#include "key-value.h"
#include "channel-map.h"

#ifndef __GNUC__
#include <fcntl.h>
#include <unistd.h>
#endif

#define INPUT_ABI_VERSION                                   1

typedef struct _InputPluginOps                              InputPluginOps;
typedef struct _InputPluginOpt                              InputPluginOpt;
typedef struct _InputPluginData                             InputPluginData;

enum
{
    INPUT_ERROR_SUCCESS,                                    // no error
    INPUT_ERROR_ERRNO,                                      // system error (error code in errno)
    INPUT_ERROR_UNRECOGNIZED_FILE_TYPE,                     // file type not recognized
    INPUT_ERROR_UNSUPPORTED_FILE_TYPE,                      // file type recognized, but not supported
    INPUT_ERROR_FUNCTION_NOT_SUPPORTED,                     // function not supported (usually seek)
    INPUT_ERROR_FILE_FORMAT,                                // input plugin detected corrupted file
    INPUT_ERROR_INVALID_URI,                                // malformed uri
    INPUT_ERROR_SAMPLE_FORMAT,                              // sample format not supported
    INPUT_ERROR_WRONG_DISC,                                 // wrong disc inserted
    INPUT_ERROR_NO_DISC,                                    // could not read disc
    INPUT_ERROR_HTTP_RESPONSE,                              // error parsing response line / headers
    INPUT_ERROR_HTTP_STATUS,                                // usually 404
    INPUT_ERROR_HTTP_REDIRECT_LIMIT,                        // too many redirections
    INPUT_ERROR_NOT_OPTION,                                 // plugin does not have this option
    INPUT_ERROR_INTERNAL                                    //
};

struct _InputPluginData
{
    int                             fd;
    char*                           fileName;

    unsigned int                    remote : 1;
    unsigned int                    metaDataChanged : 1;

    int                             counter;
    int                             metaInt;

    char*                           metaData;
    char*                           icyName;
    char*                           icyGenre;
    char*                           icyURL;

    SampleFormat                    sf;
    ChannelPosition                 channelMap[CHANNELS_MAX];
    void*                           private;
};

struct _InputPluginOps
{
    int     (*Open)                 (InputPluginData* data);
    int     (*Close)                (InputPluginData* data);
    int     (*Read)                 (InputPluginData* data, char* buffer, int count);
    int     (*Seek)                 (InputPluginData* data, double offset);
    int     (*ReadComments)         (InputPluginData* data, KeyValue** comments);
    int     (*Duration)             (InputPluginData* data);
    long    (*Bitrate)              (InputPluginData* data);
    long    (*BitrateCurrent)       (InputPluginData* data);
    char*   (*Codec)                (InputPluginData* data);
    char*   (*CodecProfile)         (InputPluginData* data);
};

struct _InputPluginOpt
{
    const char*                     name;
    int     (*Set)                  (const char* val);
    int     (*Get)                  (char** val);
};

extern const InputPluginOps         gInputOps;
extern const int                    gInputPriority;
extern const char* const            gInputExtensions[];
extern const char* const            gInputMimeTypes[];
extern const InputPluginOpt         gInputOptions[];
extern const unsigned               gInputVersion;

#endif //GRACEFUL_MUSIC_INPUT_BASE_H
