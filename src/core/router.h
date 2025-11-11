//
// Created by AWAY on 25-10-14.
//

#pragma once
#include <iostream>
#include <string>
#include <set>
#include "errc.h"
#include "leptjson.h"

namespace uno
{
    void load_role_req(int uid,
                       const std::string& callback,
                       const lept_value* callback_data);
    void create_role_req(int uid,
                        const std::string&  nick,
                        const std::string& callback,
                        const lept_value* callback_data);
    void save_role_req(int uid,
                        const lept_value& data,
                        const lept_value& summary,
                        const std::string& callback,
                        const lept_value* callback_data);
    void load_role_rsp(ErrCode ret,
                       const std::string& nickname,
                       const lept_value& summary,
                       const lept_value& data,
                       int uid);
    void create_role_rsp(ErrCode ret, int uid);
    void round_flow(int uid, int state);
    void save_role_rsp(ErrCode ret, int uid);


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

#define CALL_FN(func) if(funcname == #func) { std::invoke(func, std::forward<Args>(args)...);  }

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




