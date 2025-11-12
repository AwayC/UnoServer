//
// Created by AWAY on 25-10-14.
//

#pragma once
#include <iostream>
#include <string>
#include <set>
#include "errc.h"
#include "leptjson.h"
#include "types.h"
#include "types.h"

namespace uno
{
    class Session;
    using SessionPtr = std::shared_ptr<Session>;

    /**
     * s2s
     */
    namespace S2S
    {
        void load_role_req(int uid,
                       const std::string& callback,
                       int callback_data);
        void create_role_req(int uid,
                            const std::string&  nick,
                            const std::string& callback,
                            int callback_data);
        void save_role_req(int uid,
                            const data_t& data,
                            const summary_t& summary,
                            const std::string& callback,
                            int callback_data);
        void load_role_rsp(ErrCode ret,
                           const std::string& nickname,
                           const summary_t& summary,
                           const data_t& data,
                           int uid);
        void create_role_rsp(ErrCode ret, int uid);
        void round_flow(int uid, int state);
        void save_role_rsp(ErrCode ret, int uid);
    }


    /**
     * c2s
     */
    namespace C2S
    {
        void logout_req(SessionPtr& ss);
        void login_req(SessionPtr& ss, const std::string& token);
        void create_role_req(SessionPtr& ss, const std::string& nick);
        void create_role_req(SessionPtr& ss,
                            const std::string& title,
                            int player_count);
        void enter_room_req(SessionPtr& ss, int room_id, bool re_enter);
        void leave_room_req(SessionPtr& ss, int room_id);
        void get_ready_req(SessionPtr& ss, int room_id);
        void shuffle_room_req(SessionPtr& ss, int room_id);
        void room_use_voice_req(SessionPtr& ss, int room_id, int voice_id);
        void room_kick_player_req(SessionPtr& ss, int room_id, bool be_kicked);
        void start_game_req(SessionPtr& ss, int room_id);
        void game_play_req(SessionPtr& ss, int room_id, double c,
                            bool with_uno, int chg_color);
        void game_deal_card_req(SessionPtr& ss, int room_id);
        void game_report_no_uno_req(SessionPtr& ss, int room_id);
        void get_room_list_req(SessionPtr& ss);

    }


    class Router
    {
    public:
        static Router& router()
        {
            static Router instance;
            return instance;
        }

        std::set<std::string> c2s;
        std::set<std::string> s2s;

        template<typename... Args>
        void call_s2s(const std::string& funcname, Args&&... args)
        {
            if (s2s.find(funcname) == s2s.end())
            {
                std::cerr << "Unabled internal msg " << funcname << std::endl;
                return ;
            }

#define CALL_FN(func) if(funcname == #func) { std::apply(S2S::func, std::forward<Args>(args)...);  }

            try
            {
                CALL_FN(load_role_req)
                CALL_FN(create_role_req)
                CALL_FN(save_role_req)
                CALL_FN(load_role_rsp)
                CALL_FN(create_role_rsp)
                CALL_FN(round_flow)
                CALL_FN(save_role_rsp)
            } catch (const std::exception& e)
            {
                std::cerr << "Error while dispatch s2s msg " << funcname << std::endl;
                std::cerr << e.what() << std::endl;
            }

#undef CALL_FN(func)
        }

        template<typename... Args>
        void call_c2s(const std::string& funcname, Args&&... args)
        {
            if (c2s.find(funcname) == c2s.end())
            {
                std::cerr << "Unabled internal msg " << funcname << std::endl;
                return ;
            }

#define CALL_FN(func) if(funcname == #func) { std::apply(C2S::func, std::forward<Args>(args)...);  }

            try
            {
                CALL_FN(logout_req)
                CALL_FN(login_req)
                CALL_FN(create_role_req)
            } catch (const std::exception& e)
            {
                std::cerr << "Error while dispatch s2s msg " << funcname << std::endl;
                std::cerr << e.what() << std::endl;
            }

#undef CALL_FN(func)
        }


        bool contain_s2s(const std::string& funcname)
        {
            return (s2s.find(funcname) == s2s.end()) == false;
        }

        bool contain_c2s(const std::string& funcname)
        {
            return (c2s.find(funcname) == c2s.end()) == false;
        }


    private:
        static bool is_created;

        Router()
        {
            is_created = true;
            s2s = {
                "load_role_req",
                "create_role_req",
                "save_role_req",
                "load_role_rsp",
                "create_role_rsp",
                "round_flow",
                "save_role_rsp",
            };
        }
        ~Router() = default;

    };
}




