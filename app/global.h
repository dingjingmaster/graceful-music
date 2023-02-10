//
// Created by dingjing on 2/9/23.
//

#ifndef GRACEFUL_MUSIC_GLOBAL_H
#define GRACEFUL_MUSIC_GLOBAL_H
#include <glib.h>

typedef unsigned int                SampleFormat;
typedef struct _InputPlugins        InputPlugins;
typedef GList                       OutputPlugins;

extern char*                        gLogPath;
extern char*                        gCharset;

extern const char*                  gHomeDir;
extern const char*                  gConfigDir;
extern const char*                  gSocketPath;
extern const char*                  gRuntimeDir;
extern const char*                  gPlaylistDir;

extern InputPlugins*                gInputPlugins;
extern OutputPlugins*               gOutputPlugins;     // list of OutPlugin


/**
 * @brief
 *  解码器
 *
 * @param plugins: 所有支持的解码器
 * @param pluginIndex: 解码器支持的文件扩展名
 */
struct _InputPlugins
{
    GList*              plugins;                        // list of InputPlugin
    // FIXME:// filter plugins by name ?
    GHashTable*         pluginExtIndex;
    GHashTable*         pluginMimeIndex;
};


#endif //GRACEFUL_MUSIC_GLOBAL_H
