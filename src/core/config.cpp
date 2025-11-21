//
// Created by AWAY on 25-11-20.
//

#include "config.h"
#include <fstream>
#include <sstream>

namespace uno
{
    lept_value g_config;

    void load_config()
    {
        std::ifstream infile(UNO_CONFIG_FILE);

        if (!infile.is_open())
        {
            throw std::runtime_error("uno::load_config: failed to open config file");
        }

        std::stringstream buffer;
        buffer << infile.rdbuf();

        g_config.parse(buffer.str());

        infile.close();
        return ;
    }
}
