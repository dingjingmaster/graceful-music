//
// Created by dingjing on 2/10/23.
//

#include "interface.h"

#include "../global.h"


static const char* gInputPluginName[] = {
    "flac",
    NULL
};


void input_plugin_register()
{
    REGISTER_INPUT_PLUGIN(flac);
}

void output_plugin_register()
{

}
