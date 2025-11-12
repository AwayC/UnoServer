//
// Created by AWAY on 25-10-14.
//

#pragma once

#include "session.h"
#include "router.h"
#include <unordered_map>

#define SESSION_ITER_COUNT    10
#define SESSION_SAVE_DURATION (5 * 60 * 1000)

namespace uno
{
    using SessionPtr = std::shared_ptr<Session>;
    class Ssmgr {

        void accept(WsSessionPtr& socket);

    private:

        std::vector<SessionPtr> m_sessions;
        std::unordered_map<int, SessionPtr> m_uid2session;
        size_t m_iter = 0;

        void on_error(const SessionPtr& session, const std::exception& err)
        {
            std::cerr << err.what() << std::endl;
        }

        void on_disconnect(SessionPtr& session, const std::string& reason);

        template<typename... Args>
        void on_c2s_msg(const SessionPtr& session,
                        const std::string& funcname,
                        Args&&... args)
        {
            if (Router::router().contain_c2s(funcname))
            {
                std::cerr << "Undefined msg: " << funcname << ", uid: " << session->uid() << std::endl;
                return ;
            }

            try
            {
                Router::router().call_c2s(funcname, session, std::forward<Args>(args)...);
            } catch (const std::exception& e)
            {
                std::cerr << "Error while dispatch c2s msg" << std::endl;
                std::cerr << e.what() << std::endl;
            }

        }

        void on_attach(SessionPtr& session)
        {
            assert(!m_uid2session.contains(session->uid()));
            m_uid2session[session->uid()] = session;
        }

        void on_message(con);
    };

}

