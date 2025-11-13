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

    void Ssmgr::on_c2s_msg(SessionPtr& session, const lept_value& args)
    {
        if (args.get_type() != lept_type::array)
        {
            std::cerr << "Invalid c2s msg" << std::endl;
            return ;
        }

        std::string funcname;
        try
        {
            funcname = args.get_element(0).get_string();

            if (funcname == "logout_req")
            {
                C2S::logout_req(session);
            }
            else if (funcname == "login_req")
            {
                const std::string& token = args.get_element(1).get_string();
                C2S::login_req(session, token);
            }
            else if (funcname == "create_role_req")
            {
                const std::string& nick = args.get_element(1).get_string();
                C2S::create_role_req(session, nick);
                C2S::create_role_req(session, nick);
            }
            else if (funcname == "create_room_req")
            {
                const std::string& title = args.get_element(1).get_string();
                int play_count = args.get_element(2).get_integer();
                C2S::create_room_req(session, title, play_count);
            }
            else if (funcname == "enter_room_req")
            {
                int room_id = args.get_element(1).get_integer();
                bool re_enter = args.get_element(2).get_boolean();
                C2S::enter_room_req(session, room_id, re_enter);
            }
            else if (funcname == "leave_room_req")
            {
                int room_id = args.get_element(1).get_integer();
                C2S::leave_room_req(session, room_id);
            }
            else if (funcname == "get_ready_req")
            {
                int room_id = args.get_element(1).get_integer();
                C2S::get_ready_req(session, room_id);
            }
            else if (funcname == "shuffle_room_req")
            {
                int room_id = args.get_element(1).get_integer();
                C2S::shuffle_room_req(session, room_id);
            }
            else if (funcname == "room_use_voice_req")
            {
                int room_id = args.get_element(1).get_integer();
                int voice_id = args.get_element(2).get_integer();
                C2S::room_use_voice_req(session, room_id, voice_id);
            }
            else if (funcname == "room_kick_player_req")
            {
                int room_id = args.get_element(1).get_integer();
                bool be_kicked = args.get_element(2).get_boolean();
                C2S::room_kick_player_req(session, room_id, be_kicked);
            }
            else if (funcname == "start_game_req")
            {
                int room_id = args.get_element(1).get_integer();
                C2S::start_game_req(session, room_id);
            }
            else if (funcname == "game_play_req")
            {
                int room_id = args.get_element(1).get_integer();
                double c = args.get_element(2).get_number();
                bool with_uno = args.get_element(3).get_boolean();
                int chg_color = args.get_element(4).get_integer();
                C2S::game_play_req(session, room_id, c, with_uno, chg_color);
            }
            else if (funcname == "game_deal_card_req")
            {
                int room_id = args.get_element(1).get_integer();
                C2S::game_deal_card_req(session, room_id);
            }
            else if (funcname == "game_report_no_uno_req")
            {
                int room_id = args.get_element(1).get_integer();
                C2S::game_report_no_uno_req(session, room_id);
            }
            else if (funcname == "get_room_list_req")
            {
                C2S::get_room_list_req(session);
            } else
            {
                std::cerr << "Undefined c2s msg: " << funcname << "uid: " << session->uid() << std::endl;
            }

        } catch (const std::exception& e)
        {
            std::cerr << "Error while dispatch c2s msg: " << e.what() << std::endl;
        }
    }



}