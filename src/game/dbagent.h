//
// Created by AWAY on 25-11-10.
//

#pragma once

#include "../core/errc.h"
#include "../core/db.h"
#include "../core/router.h"

namespace uno {

    class DBagent
    {
    public:
        DBagent(DataBase* db) : m_db(db)
        {
            assert(m_db);

            std::function load_role_req = [m_db](int uid,
                                    const std::string& callback,
                                    int callback_data)
            {
                m_db->uno_game_players_load_role(uid, [=](BackResult<lept_value> result)
                {
                    try
                    {
                        lept_value data = std::get<lept_value>(result);
                        if (data.get_type() == lept_type::null)
                        {
                            Router::arg_t args = {
                                (int)ErrCode::db_not_exists,
                                "",
                                lept_value::object_t(),
                                lept_value::object_t(),
                                callback_data
                            };
                            Router::router().call_s2s(callback, args);

                            return;
                        }

                        Router::arg_t args = {
                            (int)ErrCode::ok,
                            data["nickname"],
                            data["summary"],
                            data["data"],
                            callback_data
                        };
                        Router::router().call_s2s(callback, args);

                    } catch (const std::exception& e)
                    {
                        std::cerr << "Unexpected exception while loading role" << std::endl;
                        std::cerr << e.what() << std::endl;

                        Router::arg_t args = {
                            (int)ErrCode::api_db_error,
                            "",
                            lept_value::object_t(),
                            lept_value::object_t(),
                            callback_data
                        };
                        Router::router().call_s2s(callback, args);
                    }

                });
            };

            std::function create_role_req = [m_db](int uid,
                                std::string& nick,
                                const std::string& callback,
                                int callback_data)
            {

            };

            std::function save_role_req = [m_db](int uid,
                                    lept_value::object_t data,
                                    lept_value::object_t summary,
                                    const std::string& callback,
                                    int callback_data)
            {

            };

            Router::router().register_c2s("load_role_req", load_role_req);
            Router::router().register_c2s("create_role_req", create_role_req);
            Router::router().register_c2s("save_role_req", save_role_req);
        }
        ~DBagent() = default;

    private:
        DataBase* m_db;
    };

};
