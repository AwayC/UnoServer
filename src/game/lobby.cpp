//
// Created by AWAY on 25-11-19.
//

#include "lobby.h"
#include "websocket.h"
#include "core/config.h"

namespace uno
{
#define SSMGR  Ssmgr::instance()
    static lept_value::object_t get_base_data(SessionPtr ss)
    {
        return {
            {"uid", ss->uid()},
            {"name", ss->name()},
            {"nick", ss->nick()},
            {"email", ss->email()},

            {"room_id", ss->data()["room_id"].get_integer()},
        };
    }

    namespace C2S
    {
        void logout_req(SessionPtr ss)
        {
            std::cout << "User try to logout" << std::endl;
            Ssmgr::instance().detach(ss, "logout_req");
        }

        void login_req(SessionPtr ss, std::string token)
        {
            std::cout << "User try to login" << std::endl;

            if (ss->state() != Session::State::connected)
            {
               std::cerr << "Invalid login message" << std::endl;
               ss->call("login_rsp", ErrCode::api_invalid_call);
               return ;
            }

            lept_value payload;
            try
            {
               payload = JwtUtil::verify(token, g_config["secret"].get_string(), 12);
            }
            catch (const std::exception& e)
            {
               std::cerr << e.what() << std::endl;
               ss->call("login_rsp", ErrCode::api_bad_token);
               return ;
            }

            int uid = ss->data()["uid"].get_integer();
            const std::string& username = ss->data()["username"].get_string();
            const std::string& email = ss->data()["email"].get_string();

            if (Ssmgr::instance().has_session(uid))
            {
               std::cout << "Session already logined, kick out, uid: " << uid << std::endl;
               SSMGR.detach(SSMGR.find_session(uid), "kick-out");
            }

            ss->attach(uid, username, email);
            ss->set_state(Session::State::authed);

            ss->call("load_role_req", uid, "load_role_rsp", uid);
        }

        void create_role_req(SessionPtr ss, std::string nick)
        {
            std::cout << "User try to create role" << std::endl;

            if (ss->state() != Session::State::create_role)
            {
               std::cerr << "Invalid create role message" << std::endl;
               ss->call("create_role_rsp", ErrCode::api_invalid_call);
               return ;
            }

            if (nick.length() > 10)
            {
               ss->call("create_role_rsp", (int)ErrCode::api_nick_too_long);
               return ;
            }

            if (nick.length() < 3)
            {
               ss->call("create_role_rsp", (int)ErrCode::api_nick_too_short);
               return ;
            }

            std::wregex nick_regex(
               L"^[a-zA-Z0-9_\\u3040-\\u30ff\\u3400-\\u4dbf\\u4e00-\\u9fff\\uf900-\\ufaff\\uff66-\\uff9f]+$",
               std::regex::icase // icase 选项模拟 JS 的 /i 标志
            );

            std::wstring w_nick(nick.begin(), nick.end());

            if (!std::regex_match(w_nick, nick_regex)) {
               ss->call("create_role_rsp", (int)ErrCode::api_nick_invalid);
               return;
            }

            CALL_S2S("create_role_req", ss->uid(), nick, "create_role_rsp", ss->uid());
            ss->set_state(Session::State::create_role_wait);
        }

    }

    namespace S2S
    {

        void load_role_rsp(int ret, std::string nick,
                       lept_value::object_t summary,
                       lept_value::object_t data,
                       int uid)
        {
            SessionPtr ss = SSMGR.find_session(uid);
            if (!ss)
                return ;

            if (ret == static_cast<int>(ErrCode::db_not_exists))
            {
                std::cout << "Have to create role for uid: " << uid << std::endl;
                ss->call("create_role_ntf");
                ss->set_state(Session::State::create_role);
            }

            if (ret != static_cast<int>(ErrCode::ok))
            {
                std::cerr << "Load role data error, uid: " << uid << ", code " << ret << std::endl;
                ss->call("login_rsp", ErrCode::api_db_error);
            }

            ss->attach_data(nick,
                lept_value(std::move(summary)),
                lept_value(std::move(data)));
            ss->set_state(Session::State::logged);

            // evtcenter.emit("login", ss);

            std::string token = JwtUtil::sign({
                {"uid", uid},
                {"username", ss->name()},
                {"email", ss->email()}
            }, g_config["secret"].get_string(), 12 * 60 * 60);
            ss->call("login_rsp", static_cast<int>(ErrCode::ok), token, get_base_data(ss));
        }

        void create_role_rsp(int ret, int uid)
        {
            SessionPtr ss = SSMGR.find_session(uid);
            if (!ss)
                return ;

            if (ret == int(ErrCode::api_nick_in_use))
            {
                ss->set_state(Session::State::create_role);
                ss->call("create_role_rsp", (int)ErrCode::api_nick_in_use);
                return ;
            }

            if (ret != int(ErrCode::ok))
            {
                ss->set_state(Session::State::create_role);
                ss->call("create_role_rsp", (int)ErrCode::api_db_error);
                return ;
            }

            ss->call("create_role_rsp", (int)ErrCode::ok);

            ss->set_state(Session::State::authed);
            CALL_S2S("load_role_req", uid, "load_role_rsp", uid);
        }

        void save_role_rsp(int ret, int uid)
        {
            if (ret != (int)ErrCode::ok)
            {
                std::cerr << "Failed to save role data for uid: " << uid << std::endl;
            }
        }
    }

    Lobby::Lobby()
    {
#define REGISTER_FUNC_S2S(func) \
        REGISTER_S2S(#func, S2S::func)

#define REGISTER_FUNC_C2S(func) \
        REGISTER_C2S(#func, C2S::func)

        REGISTER_FUNC_S2S(load_role_rsp);
        REGISTER_FUNC_S2S(create_role_rsp);
        REGISTER_FUNC_S2S(save_role_rsp);

        REGISTER_FUNC_C2S(create_role_req);
        REGISTER_FUNC_C2S(login_req);
        REGISTER_FUNC_C2S(logout_req);


#undef REGISTER_FUNC_S2S


    }

}