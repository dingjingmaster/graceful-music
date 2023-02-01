//
// Created by dingjing on 2/1/23.
//

#include "key-value.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

KeyValue* key_value_new (int num)
{
    KeyValue* c = (KeyValue*) malloc (sizeof (KeyValue) * (num + 1));
    if (!c) {
        // error
        return NULL;
    }

    for (int i = 0; i <= num; ++i) {
        c[i].key = NULL;
        c[i].value = NULL;
    }

    return c;
}

void key_value_init (GrowingKeyValues* c, const KeyValue* keyValues)
{
    int i = 0;

    for (i = 0; keyValues[i].key; ++i);

    c->keyValues = key_value_dup (keyValues);
    c->alloc = i;
    c->count = i;
}

void key_value_add (GrowingKeyValues* c, const char* key, char* val)
{
    int n = c->count + 1;

    if (n > c->alloc) {
        n = (n + 3) & ~3;
        c->keyValues = (KeyValue*) realloc (c->keyValues, sizeof (KeyValue) * n);
        c->alloc = n;
    }

    c->keyValues[c->count].key = strdup (key);
    c->keyValues[c->count].value = val;
    ++c->count;
}

const char* key_value_get_val_growing (const GrowingKeyValues* c, const char* key)
{
    for (int i = 0; i < c->count; ++i) {
        if (0 == strcasecmp (c->keyValues[i].key, key)) {
            return c->keyValues[i].value;
        }
    }

    return NULL;
}

void key_value_terminate (GrowingKeyValues* c)
{
    int alloc = c->count + 1;

    if (alloc > c->alloc) {
        c->keyValues = (KeyValue*) realloc (c->keyValues, sizeof (KeyValue) * alloc);
        c->alloc = alloc;
    }

    c->keyValues[c->count].key = NULL;
    c->keyValues[c->count].value = NULL;
}

void key_value_free (KeyValue* keyValues)
{
    for (int i = 0; keyValues[i].key; ++i) {
        free (keyValues[i].key);
        free (keyValues[i].value);
    }

    free (keyValues);
}

KeyValue* key_value_dup (const KeyValue* keyValues)
{
    int i = 0;
    KeyValue* c = NULL;

    for (i = 0; keyValues[i].key; ++i);

    c = (KeyValue*) malloc (sizeof (KeyValue) * (i + 1));
    if (!c) {
        // error
        return NULL;
    }

    for (i = 0; keyValues[i].key; ++i) {
        c[i].key = (char*) strdup (keyValues[i].key);
        c[i].value = (char*) strdup (keyValues[i].value);
    }

    c[i].key = NULL;
    c[i].value = NULL;

    return c;
}

const char* key_value_get_value (const KeyValue* keyValues, const char* key)
{
    for (int i = 0; keyValues[i].key; ++i) {
        if (0 == strcasecmp (keyValues[i].key, key)) {
            return keyValues[i].value;
        }
    }

    return NULL;
}
