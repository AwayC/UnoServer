//
// Created by AWAY on 25-10-14.
//

#include "ssmgr.h"


namespace uno
{
    void Ssmgr::on_disconnect(SessionPtr& session, const std::string& reason)
    {
        for (size_t i = 0;i < m_sessions.size(); ++ i)
        {
            if (m_sessions[i] == session)
            {
                std::cout << "Session removed due to" << reason << std::endl;

                if (session->uid() != -1)
                {
                    m_uid2session.erase(session->uid());
                }

                m_sessions[i] = m_sessions.back();
                m_sessions.pop_back();

                //todo: evtcenter.emit('logout', session, reason);
                return ;
            }
        }
    }

    void Ssmgr::accept(WsSessionPtr& socket)
    {
        auto ss = std::make_shared<Session>(socket);

        auto weak_ss = std::weak_ptr<Session>(ss);
        socket->onMessage([this, weak_ss](WsSessionPtr socket)
        {
            if (auto ss = weak_ss.lock())
            {
                lept_value args = socket->getJsonMessage();
                try
                {
                    auto evt = args.get_array_element(0).get_string();
                    if (evt == "c2s")
                    {
                        // on_c2s_msg(ss, args);
                    }
                } catch (const std::exception& err)
                {
                    std::cerr << err.what() << std::endl;
                }
            }
        });

        socket->onError([this, weak_ss](WsSessionPtr socket, const std::exception& err)
        {
            if (auto ss = weak_ss.lock())
            {
                on_error(ss, err);
            }
        });

        socket->onClose([this, weak_ss](const WsSessionPtr& socket)
        {
            if (auto ss = weak_ss.lock())
            {
                on_disconnect(ss, "close");
            }
        });


        m_sessions.emplace_back(ss);
    }


}