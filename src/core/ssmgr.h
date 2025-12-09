//
// Created by AWAY on 25-10-14.
//

#pragma once

#include "session.h"
#include "router.h"
#include <unordered_map>
#include <chrono>

#define SESSION_ITER_COUNT    10
constexpr auto SESSION_SAVE_DURATION = std::chrono::seconds(5 * 60 * 1000);

namespace uno
{
    using SessionPtr = std::shared_ptr<Session>;
    class Ssmgr {
    public:
        static Ssmgr& instance()
        {
            static Ssmgr ssmgr; return ssmgr;
        }

        void accept(WsSessionPtr& socket);

        void detach(SessionPtr session, const std::string& reason)
        {
            session->call("logout_ntf", reason);
            // on_disconnect(session, reason);
            if (auto ws = session->socket().lock())
            {
                ws->close();
                return ;
            }

            on_disconnect(session, reason);
        }

        bool has_session(int uid)
        {
            return m_uid2session.contains(uid);
        }

        SessionPtr find_session(int uid)
        {
            auto it = m_uid2session.find(uid);
            if (it == m_uid2session.end())
                return nullptr;

            return it->second;
        }

        size_t get_session_count()
        {
            return m_uid2session.size();
        }

        void update_session(SessionPtr ss, const std::chrono::system_clock::time_point& now)
        {
            //todo
            auto time_since_last_save = now - ss->last_save_time();

            if (ss->state() >= Session::State::logged && ss->dirty()
                && time_since_last_save >= SESSION_SAVE_DURATION)
            {
                std::cout << "update_session: auto save session data for " << ss->uid() << std::endl;

                Router::arg_t args = {ss->uid(), ss->data(), ss->summary(), "", 0};
                Router::router().call_s2s("save_role_req", args);
                ss->set_undirty(now);
            }
        }

        void update(const std::chrono::system_clock::time_point& now)
        {
            if (!m_sessions.empty())
            {
                if (m_iter >= m_sessions.size())
                {
                    m_iter = 0;
                }

                size_t begin = m_iter;
                size_t cnt = std::max((size_t)1, m_sessions.size() / SESSION_ITER_COUNT);
                for (size_t i = 0;i < cnt; ++ i)
                {
                    auto& ss = m_sessions[i];
                    assert(ss);
                    try
                    {
                        update_session(ss, now);
                    } catch (const std::exception& err)
                    {
                        std::cerr << "update: Unexpected error while update session" << std::endl;
                        std::cerr << err.what() << std::endl;
                    }

                    m_iter ++;
                    if (m_iter >= m_sessions.size())
                    {
                        m_iter = 0;
                    }

                    if (m_iter == begin)
                    {
                        break;
                    }
                }
            }
        }

    private:

        std::vector<SessionPtr> m_sessions;
        std::unordered_map<int, SessionPtr> m_uid2session;
        size_t m_iter = 0;

        Ssmgr() = default;

        void on_error(SessionPtr session, const std::exception& err)
        {
            std::cerr << err.what() << std::endl;
        }

        void on_disconnect(SessionPtr session, const std::string& reason);

        void on_c2s_msg(SessionPtr session, Router::arg_t& args);

        void on_attach(SessionPtr session)
        {
            assert(!m_uid2session.contains(session->uid()));
            m_uid2session[session->uid()] = session;
        }

    };

}

