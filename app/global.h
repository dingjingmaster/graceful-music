//
// Created by dingjing on 2/9/23.
//

#ifndef GRACEFUL_MUSIC_GLOBAL_H
#define GRACEFUL_MUSIC_GLOBAL_H
#include <glib.h>

typedef unsigned int                SampleFormat;
typedef struct _InputPlugins        InputPlugins;

extern char*                        gLogPath;
extern char*                        gCharset;

extern const char*                  gHomeDir;
extern const char*                  gConfigDir;
extern const char*                  gSocketPath;
extern const char*                  gRuntimeDir;
extern const char*                  gPlaylistDir;

extern InputPlugins*                gInputPlugins;



/**
 * @brief
 *  解码器
 *
 * @param plugins: 所有支持的解码器
 * @param pluginIndex: 解码器支持的文件扩展名
 */
struct _InputPlugins
{
    GList*              plugins;
    GHashTable*         pluginIndex;
};


#endif //GRACEFUL_MUSIC_GLOBAL_H
