//
// Created by dingjing on 2/10/23.
//

#ifndef GRACEFUL_MUSIC_INTERFACE_H
#define GRACEFUL_MUSIC_INTERFACE_H

#include "input-interface.h"
#include "mixer-interface.h"
#include "output-interface.h"

#include "log.h"
#include "../global.h"

#include <stdbool.h>

#define REGISTER_INPUT_PLUGIN(name) \
{                                                                               \
    InputPlugin* name##ip = (InputPlugin*) g_malloc0 (sizeof (InputPlugin));    \
    name##_input_register(name##ip);                                            \
    if (!name##ip->ops) {                                                       \
        g_free (name##ip);                                                      \
    }                                                                           \
    else {                                                                      \
        GList* name##l = gInputPlugins->plugins;                                \
        GHashTable* name##Ext = gInputPlugins->pluginExtIndex;                  \
        GHashTable* name##M = gInputPlugins->pluginMimeIndex;                   \
        gInputPlugins->plugins = g_list_append (name##l, name##ip);             \
        const char* const* name##extensions = name##ip->extensions;             \
        for (int i = 0; NULL != name##extensions[i]; ++i) {                     \
            g_hash_table_insert (name##Ext, name##extensions[i], name##ip);     \
        }                                                                       \
                                                                                \
        const char* const* name##Mime = name##ip->mimeType;                     \
        for (int i = 0; NULL != name##Mime[i]; ++i) {                           \
            g_hash_table_insert (name##M, name##Mime[i], name##ip);             \
        }                                                                       \
        LOG_DEBUG ("register input plugin '%s' ok", #name);                     \
    }                                                                           \
}

struct _InputPlugin
{
    /* 插件需要提供的功能 - 开始 */
    bool                                isExtension;
    char*                               name;
    int                                 priority;
    const char* const*                  mimeType;           // 以 NULL 结束
    const char* const*                  extensions;         // 以 NULL 结束
    const InputPluginOps*               ops;
    const InputPluginOpt*               option;
    unsigned int                        abiVersion;
    /* 插件需要提供的功能 - 结束 */

    InputPluginData                     data;
    unsigned int                        open : 1;
    unsigned int                        eof : 1;
    int                                 httpCode;
    char*                               httpReason;

    int                                 duration;       // cached duration, -1 = unset
    long                                bitrate;        // cached bitrate, -1 = unset
    char*                               codec;          // cached codec, NULL = unset
    char*                               codecProfile;   // cached codec_profile, NULL = unset

    /*
     * pcm is converted to 16-bit signed little-endian stereo
     * NOTE: no conversion is done if channels > 2 or bits > 16
     */
    void (*PcmConvert)          (void*, const void*, int);
    void (*PcmConvertInPlace)   (void*, int);
    /*
     * 4  if 8-bit mono
     * 2  if 8-bit stereo or 16-bit mono
     * 1  otherwise
     */
    int                                 pcmConvertScale;
};


struct _OutputPlugin
{
    char*                               name;
    void*                               handle;

    const unsigned int                  abiVersion;
    const OutputPluginOps*              pcmOps;
    const MixerPluginOps*               mixerOps;
    const OutputPluginOpt*              pcmOptions;
    const MixerPluginOpt*               mixerOptions;
    int                                 priority;

    unsigned int                        pcmInitialized : 1;
    unsigned int                        mixerInitialized : 1;
    unsigned int                        mixerOpen : 1;
};

void input_plugin_register ();

void output_plugin_register ();

#endif //GRACEFUL_MUSIC_INTERFACE_H