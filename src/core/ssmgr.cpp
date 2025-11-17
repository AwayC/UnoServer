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
                lept_value json = socket->getJsonMessage();

                if (!json.is<lept_value::array_t>())
                {
                    std::cerr << "session msg error" << std::endl;
                    return;
                }

                auto& args = json.get<lept_value::array_t>();

                assert(args.size() < 2);

                try
                {
                    std::string& evt = args[0].get<std::string>();
                    if (evt == "c2s")
                        on_c2s_msg(ss, args);
                } catch (const std::exception& e)
                {

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

    void Ssmgr::on_c2s_msg(SessionPtr& session, const Router::arg_t& args)
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
            newargs.push_back(args[i]);
        }

        Router::router().call_c2s(funcname, session, newargs);
    }



}