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

        socket->onMessage([this, ])

        m_sessions.emplace_back(ss);
    }


}