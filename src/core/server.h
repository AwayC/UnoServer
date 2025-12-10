//
// Created by AWAY on 25-10-14.
//

#pragma once
#include <iostream>
#include "errc.h"
#include "db.h"
#include "background.h"
#include "httpserver.h"
#include "WebSocket.h"
#include "WsServer.h"
#include "leptjson.h"
#include "game/dbagent.h"
#include "game/lobby.h"
#include "game/room.h"

#define UNO_DB_PATH "uno_game.db3"
#define UNO_SERVER_PORT 8081
#define UNO_SERVER_IP "127.0.0.1"

using HttpServerPtr = std::shared_ptr<HttpServer>;

namespace uno
{
    class Server
    {
    public:
        Server(lept_value& cfg);


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

        struct
        {
            uv_loop_t* loop;
            uv_async_t async;
            ThreadQue<BackCallback> que;
        } m_loop_ctx;

        DataBase m_db;
        DBagent m_dbagent;
        Background m_bg;
        uv_timer_t m_updateTimer;


        void internalLogin(const std::string& name,
                        const std::string& password,
                        const std::string& ip,
                        const std::function<void(BackResult<std::pair<ErrCode, std::string>>)>& cb);

        void registerRouter();

        void startUpdateTimer();

        void stopUpdateTimer();

        void onDbConnected();

        void onDbConnectFailed();

        void onServerListen();

        void onSocketConnection(WsSessionPtr& socket);

        void onUpdate();

        void pageIndex(httpReq* req, httpRespPtr resp);

        void pageRegister(httpReq* req, httpRespPtr resp);

        void pageLogin(httpReq* req, httpRespPtr resp);

        void pageUpdateEmail(httpReq* req, httpRespPtr resp);

        void pageUpdatePassword(httpReq* req, httpRespPtr resp);

        // background handler
        static void background_handler(uv_async_t* handle)
        {
            auto self = static_cast<Server*>(handle->data);
            while (!self->m_loop_ctx.que.empty())
            {
                auto task = self->m_loop_ctx.que.try_pop();
                if (task != std::nullopt && task.value())
                {
                    task.value()();
                }
            }
        }

    };
}
