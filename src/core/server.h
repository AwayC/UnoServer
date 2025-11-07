//
// Created by AWAY on 25-10-14.
//

#pragma once
#include <iostream>
#include "errc.h"
#include "router.h"
#include "db.h"
#include "background.h"
#include "httpserver.h"
#include "WebSocket.h"
#include "WsServer.h"
#include "leptjson.h"

#define UNO_DB_PATH "uno_game.db3"
#define UNO_SERVER_PORT 8080
#define UNO_SERVER_IP "127.0.0.1"

using HttpServerPtr = std::shared_ptr<HttpServer>;

namespace uno
{
    class Server
    {
    public:
        Server(lept_value& cfg) :
            m_cfg(cfg),
            m_staticDir("/../static/"),
            m_httpSvr(HttpServer::create( UNO_SERVER_IP, UNO_SERVER_PORT)),
            m_wsSvr(m_httpSvr),
            m_loop(m_httpSvr->getLoop())
        {
            //todo init server
        }

        void run();

    private:
        lept_value m_cfg;
        std::string m_staticDir;

        HttpServerPtr m_httpSvr;
        WsServer m_wsSvr;

        uv_loop_t* m_loop;

        void internalLogin(const std::string& name,
                        const std::string& password,
                        const std::string& ip);

        void registerRouter();

        void onDbConnected();

        void onDbConnectFailed();

        void onServerListen();

        void onUpdate();

        void pageIndex();

        void pageRegisgter();

        void pageLogin();

        void pageUpdateEmail();

        void pageUpdatePassword();

    };
}
