//
// Created by dingjing on 2/1/23.
//

#ifndef GRACEFUL_MUSIC_KEY_VALUE_H
#define GRACEFUL_MUSIC_KEY_VALUE_H

struct _KeyValue
{
    char*               key;
    char*               value;
};

struct _GrowingKeyValues
{
    struct _KeyValue*   keyValues;
    int                 alloc;
    int                 count;
};

typedef struct _KeyValue                KeyValue;
typedef struct _GrowingKeyValues        GrowingKeyValues;

#define GROWING_KEY_VALUES(name)        GrowingKeyValues name = {NULL, 0, 0}

KeyValue*       key_value_new               (int num);
void            key_value_init              (GrowingKeyValues* c, const KeyValue* keyValues);
void            key_value_add               (GrowingKeyValues* c, const char* key, char* val);
const char*     key_value_get_val_growing   (const GrowingKeyValues* c, const char* key);
void            key_value_terminate         (GrowingKeyValues* c);
void            key_value_free              (KeyValue* keyValue);
KeyValue*       key_value_dup               (const KeyValue* keyValue);
const char*     key_value_get_value         (const KeyValue* keyValue, const char* key);


#endif //GRACEFUL_MUSIC_KEY_VALUE_H
