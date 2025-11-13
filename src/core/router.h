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
                            const lept_value& data,
                            const lept_value& summary,
                            const std::string& callback,
                            int callback_data);
        void load_role_rsp(ErrCode ret,
                           const std::string& nickname,
                           const lept_value& summary,
                           const lept_value& data,
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
        void create_room_req(SessionPtr& ss,
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

        void call_s2s(const std::string& funcname, const lept_value& args);


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
            c2s = {
                "logout_req",
                "login_req",
                "create_role_req",
                "create_room_req",
                "enter_room_req",
                "leave_room_req",
                "get_ready_req",
                "shuffle_room_req",
                "room_use_voice_req",
                "room_kick_player_req",
                "start_game_req",
                "game_play_req",
                "game_deal_card_req",
                "game_report_no_uno_req",
                "get_room_list_req",
            };
        }
        ~Router() = default;

    };
}




