//
// Created by AWAY on 25-10-14.
//

#include "router.h"

namespace uno
{
    void Router::call_s2s(const std::string& funcname, const lept_value& args)
    {
        if (s2s.find(funcname) == s2s.end() || args.get_type() != lept_type::array)
        {
            std::cerr << "Unabled internal msg " << funcname << std::endl;
            return ;
        }

        try
        {
            if (funcname == "load_role_req")
            {
                int uid = args.get_element(1).get_integer();
                const std::string& callback = args.get_element(2).get_string();
                int callback_data = args.get_element(3).get_integer();
                S2S::load_role_req(uid, callback, callback_data);
            }
            else if (funcname == "create_role_req")
            {
                int uid = args.get_element(1).get_integer();
                const std::string& nick = args.get_element(2).get_string();
                const std::string& callback = args.get_element(3).get_string();
                int callback_data = args.get_element(4).get_integer();
                S2S::create_role_req(uid, nick, callback, callback_data);
            }
            else if (funcname == "save_role_req")
            {
                int uid = args.get_element(1).get_integer();
                const auto& data = args.get_element(2);
                const auto& summary = args.get_element(3);
                std::string callback = args.get_element(4).get_string();
                int callback_data = args.get_element(5).get_integer();
                S2S::save_role_req(uid, data, summary, callback, callback_data);
            }
            else if (funcname == "load_role_rsp")
            {
                ErrCode ret = static_cast<ErrCode>(args.get_element(1).get_integer());
                const std::string& nickname = args.get_element(2).get_string();
                const auto& summary = args.get_element(3);
                const auto& data = args.get_element(4);
                int uid = args.get_element(5).get_integer();
                S2S::load_role_rsp(ret, nickname, summary, data, uid);
            }
            else if (funcname == "create_role_rsp")
            {
                ErrCode ret = static_cast<ErrCode>(args.get_element(1).get_integer());
                int uid = args.get_element(2).get_integer();
                S2S::create_role_rsp(ret, uid);
            }
            else if (funcname == "round_flow")
            {
                int uid = args.get_element(1).get_integer();
                int state = args.get_element(2).get_integer();
                S2S::round_flow(uid, state);
            }
            else if (funcname == "save_role_rsp")
            {
                ErrCode ret = static_cast<ErrCode>(args.get_element(1).get_integer());
                int uid = args.get_element(2).get_integer();
                S2S::save_role_rsp(ret, uid);
            }

        } catch (const std::exception& e)
        {
            std::cout << "Error while dispatch s2s msg" << funcname << std::endl;
        }
    }
}