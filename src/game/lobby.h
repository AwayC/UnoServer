//
// Created by AWAY on 25-11-19.
//

#pragma once

#include <cassert>
#include "WebSocket.h"
#include "../core/router.h"
#include "../core/session.h"
#include "../core/errc.h"
#include "../core/ssmgr.h"

namespace uno {

    class Lobby
    {
    public:
        static Lobby& instance()
        {
            static Lobby ins;
            return ins;
        }
    private:
        Lobby();
    };


};

