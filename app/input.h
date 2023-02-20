//
// Created by dingjing on 2/10/23.
//

#ifndef GRACEFUL_MUSIC_INPUT_H
#define GRACEFUL_MUSIC_INPUT_H

#include "sf.h"
#include "key-value.h"
#include "interface.h"
#include "channel-map.h"


void ip_load_plugins(void);

/*
 * allocates new struct input_plugin.
 * never fails. does not check if the file is really playable
 */
InputPlugin* ip_new(const char *filename);

/*
 * frees struct input_plugin closing it first if necessary
 */
void ip_delete(InputPlugin* ip);

/*
 * errors: IP_ERROR_{ERRNO, FILE_FORMAT, SAMPLE_FORMAT}
 */
int ip_open(InputPlugin* ip);

void ip_setup(InputPlugin* ip);

/*
 * errors: none?
 */
int ip_close(InputPlugin* ip);

/*
 * errors: IP_ERROR_{ERRNO, FILE_FORMAT}
 */
int ip_read(InputPlugin* ip, char *buffer, int count);

/*
 * errors: IP_ERROR_{FUNCTION_NOT_SUPPORTED}
 */
int ip_seek(InputPlugin* ip, double offset);

/*
 * errors: IP_ERROR_{ERRNO}
 */
int ip_read_comments(InputPlugin* ip, KeyValue** comments);

int ip_duration(InputPlugin* ip);
int ip_bitrate(InputPlugin* ip);
int ip_current_bitrate(InputPlugin* ip);
char *ip_codec(InputPlugin* ip);
char *ip_codec_profile(InputPlugin* ip);

SampleFormat ip_get_sf(InputPlugin* ip);
void ip_get_channel_map(InputPlugin* ip, ChannelPosition* channelMap);
const char *ip_get_filename(InputPlugin* ip);
const char *ip_get_metadata(InputPlugin* ip);
int ip_is_remote(InputPlugin* ip);
int ip_metadata_changed(InputPlugin* ip);
int ip_eof(InputPlugin* ip);
void ip_add_options(void);
char *ip_get_error_msg(InputPlugin* ip, int rc, const char *arg);
char **ip_get_supported_extensions(void);
void ip_dump_plugins(void);


#endif //GRACEFUL_MUSIC_INPUT_H
