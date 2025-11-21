//
// Created by AWAY on 25-11-20.
//

#pragma once

#include "leptjson.h"

#define UNO_CONFIG_FILE "config.json"

namespace uno
{
    extern lept_value g_config;

    void load_config();
}
