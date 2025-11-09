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
            m_loop_ctx({m_httpSvr->getLoop(), {}, {}}),
            m_bg(&m_loop_ctx.que, &m_loop_ctx.async),
            m_db(&m_bg, UNO_DB_PATH)
        {
            //todo init server
            registerRouter();
        }


        const DataBase* get_db() const
        {
            return &m_db;
        }

        void run();

    private:
        lept_value m_cfg;
        std::string m_staticDir;

        HttpServerPtr m_httpSvr;
        WsServer m_wsSvr;

        DataBase m_db;
        Background m_bg;

        struct
        {
            uv_loop_t* loop;
            uv_async_t async;
            ThreadQue<BackCallback> que;
        } m_loop_ctx;


        void internalLogin(const std::string& name,
                        const std::string& password,
                        const std::string& ip,
                        std::function<void(ErrCode, std::string)> cb);

        void registerRouter();

        void onDbConnected();

        void onDbConnectFailed();

        void onServerListen();

        void onSocketConnection(const WsSessionPtr& socket);

        void onUpdate();

        void pageIndex(httpReq* req, httpRespPtr resp);

        void pageRegister(httpReq* req, httpRespPtr resp);

        void pageLogin(httpReq* req, httpRespPtr resp);

        void pageUpdateEmail(httpReq* req, httpRespPtr resp);

        void pageUpdatePassword(httpReq* req, httpRespPtr resp);

    };
}
