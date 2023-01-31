//
// Created by dingjing on 1/31/23.
//

#ifndef GRACEFUL_MUSIC_BUFFER_H
#define GRACEFUL_MUSIC_BUFFER_H

void buffer_init (void);
void buffer_free (void);

int buffer_get_read_pos  (char** pos);
int buffer_get_write_pos (char** pos);

void buffer_consume (int count);

int buffer_fill (int count);

void buffer_reset (void);

int buffer_get_filled_chunks (void);

#endif //GRACEFUL_MUSIC_BUFFER_H
