//
// Created by AWAY on 25-12-6.
//

#include "core/server.h"

int main()
{
    lept_value cfg;
    cfg.parse(R"({"port": 8081, "secret": "123456"})");
    uno::Server svr(cfg);
    svr.run();
}