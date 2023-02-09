#ifndef CMUS_MISC_H
#define CMUS_MISC_H

#include <stddef.h>

extern const char *cmus_config_dir;
extern const char *cmus_playlist_dir;
extern const char *cmus_socket_path;
extern const char *cmus_data_dir;
extern const char *cmus_lib_dir;
extern const char *home_dir;

char **get_words(const char *text);
int strptrcmp(const void *a, const void *b);
int strptrcoll(const void *a, const void *b);
int misc_init(void);
const char *escape(const char *str);
const char *unescape(const char *str);
const char *get_filename(const char *path);

/*
 * @field   contains Replay Gain data format in bit representation
 * @gain    pointer where to store gain value * 10
 *
 * Returns 0 if @field doesn't contain a valid gain value,
 *         1 for track (= radio) adjustment
 *         2 for album (= audiophile) adjustment
 *
 * http://replaygain.hydrogenaudio.org/rg_data_format.html
 */
int replaygain_decode(unsigned int field, int *gain);

char *expand_filename(const char *name);
void shuffle_array(void *array, size_t n, size_t size);

#endif
