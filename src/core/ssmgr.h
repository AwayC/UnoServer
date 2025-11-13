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

        void on_c2s_msg(SessionPtr& session, const lept_value& args);

        void on_attach(SessionPtr& session)
        {
            assert(!m_uid2session.contains(session->uid()));
            m_uid2session[session->uid()] = session;
        }

        void detach(SessionPtr& session, const std::string& reason)
        {
            session->call("logout_ntf", reason);
            on_disconnect(session, reason);

            session->socket()->close();
        }

        bool has_session(int uid)
        {
            return m_uid2session.contains(uid);
        }

        SessionPtr find_session(int uid)
        {
            return m_uid2session[uid];
        }

        size_t get_session_count()
        {
            return m_uid2session.size();
        }

        void update_session(SessionPtr& session)
        {
            //todo
        }

        void update(time_t now)
        {
            //todo
        }


    };

}

