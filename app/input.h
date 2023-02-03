//
// Created by dingjing on 2/2/23.
//

#ifndef GRACEFUL_MUSIC_INPUT_H
#define GRACEFUL_MUSIC_INPUT_H

typedef struct _InputPlugin             InputPlugin;

void input_load_plugins (void);

InputPlugin*    input_new           (const char* fileName);
int             input_close         (InputPlugin* ip);
void            input_delete        (InputPlugin* ip);
int             input_open          (InputPlugin* ip);


#endif //GRACEFUL_MUSIC_INPUT_H
