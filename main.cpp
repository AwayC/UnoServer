//
// Created by AWAY on 25-10-14.
//
#include "core/server.h"
#include "core/config.h"

int main()
{
    // uno::g_config.parse(R"({"port": 8081, "secret": "123456"})");
    uno::load_config();
    uno::Server svr(uno::g_config);
    svr.run();
}