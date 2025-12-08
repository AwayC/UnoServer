//
// Created by AWAY on 25-10-14.
//

#include "ssmgr.h"


namespace uno
{
    void Ssmgr::on_disconnect(SessionPtr session, const std::string& reason)
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

        socket->onMessage([this, ss](WsSessionPtr socket)
        {
            lept_value json = socket->getJsonMessage();

            if (!json.is<lept_value::array_t>())
            {
                on_error(ss, std::runtime_error("receive msg type is not array"));
                return;
            }

            auto& args = json.get<lept_value::array_t>();

            assert(args.size() < 2);

            try
            {
                on_c2s_msg(ss, args);
            } catch (const std::exception& e)
            {

            }

        });

        socket->onError([this, ss](WsSessionPtr socket, const std::exception& err)
        {
            on_error(ss, err);
        });

        socket->onClose([this, ss](const WsSessionPtr& socket)
        {
            on_disconnect(ss, "close");
        });

        ss->on_attach([this](SessionPtr ss)
        {
            this->on_attach(ss);
        });

        m_sessions.emplace_back(ss);
    }

    void Ssmgr::on_c2s_msg(SessionPtr session, const Router::arg_t& args)
    {
        std::string funcname;
        try
        {
            funcname = args[0].get_string();
            if (!Router::router().contain_c2s(funcname))
            {
                std::cerr << "Undefined msg: " << funcname << " uid: " << session->uid() << std::endl;
                return ;
            }
        } catch (std::exception& e)
        {
            std::cerr << "c2s funcname error" << std::endl;
        }

        Router::arg_t newargs;

        for (int i = 2;i < args.size();i ++)
        {
            newargs.push_back(std::move(args[i]));
        }

        Router::router().call_c2s(funcname, session, newargs);
    }



}