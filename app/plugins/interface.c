//
// Created by dingjing on 2/10/23.
//

#include "interface.h"

#include "input/aac.h"
#include "input/flac.h"
#include "input/ffmpeg.h"

#include "output/alsa.h"

#include <stdbool.h>


static gint output_plugin_compare (gconstpointer a, gconstpointer b);

extern OutputPlugin* gOutputPlugin;

static bool gInputRegisted = false;
static bool gOutputRegisted = false;

void input_plugin_register()
{
    REGISTER_INPUT_PLUGIN(aac);
    REGISTER_INPUT_PLUGIN(flac);
    REGISTER_INPUT_PLUGIN(ffmpeg);

    gInputRegisted = true;
}

void output_plugin_register()
{
    REGISTER_OUTPUT_PLUGIN(alsa);

    gOutputRegisted = true;
}

static gint output_plugin_compare (gconstpointer a, gconstpointer b)
{
    return ((const OutputPlugin*)b)->priority - ((const OutputPlugin*)a)->priority;
}
