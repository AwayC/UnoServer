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

            std::function load_role_req = [this](int uid,
                                    const std::string& callback,
                                    int callback_data)
            {
                m_db->uno_game_players_load_role(uid, [=](BackResult<lept_value> result)
                {
                    try
                    {
                        lept_value& data = std::get<lept_value>(result);
                        if (data.get_type() == lept_type::null)
                        {
                            CALL_S2S(callback, (int)ErrCode::db_not_exists,
                                                    "", lept_value::object_t(),
                                                    lept_value::object_t(),
                                                    callback_data);
                            return;
                        }

                        CALL_S2S(callback, (int)ErrCode::ok,
                                                data["nickname"],
                                                data["summary"],
                                                data["data"],
                                                callback_data);

                    } catch (const std::exception& e)
                    {
                        std::cerr << "Unexpected exception while loading role" << std::endl;
                        std::cerr << e.what() << std::endl;


                        CALL_S2S(callback, (int)ErrCode::api_db_error,
                                            "", lept_value::object_t(),
                                            lept_value::object_t(),
                                            callback_data);
                    }

                });
            };

            std::function create_role_req = [this](int uid,
                                std::string& nick,
                                const std::string& callback,
                                int callback_data)
            {
                auto task = [=]()
                {
                    bool exit = this->m_db->uno_game_players_has_nickname(nick);
                    if (!exit)
                    {
                        this->m_db->uno_game_players_create_role(uid, nick);
                    }

                    return exit;
                };

                m_db->get_bg()->submit(task, [=](BackResult<bool> result)
                {
                    try
                    {
                        bool exit = std::get<bool>(result);
                        if (exit)
                        {
                            CALL_S2S(callback, (int)ErrCode::api_nick_in_use, callback_data);
                            return;
                        }
                    } catch (const std::exception& e)
                    {
                        std::cerr << "Unexpected exception while creating role" << std::endl;
                        std::cerr << e.what() << std::endl;

                        CALL_S2S(callback, (int)ErrCode::api_db_error, callback_data);
                    }

                    CALL_S2S(callback, (int)ErrCode::ok, callback_data);
                });
            };

            std::function save_role_req = [this](int uid,
                                    lept_value::object_t data,
                                    lept_value::object_t summary,
                                    const std::string& callback,
                                    int callback_data)
            {
                if (data.empty() || summary.empty())
                {
                    if (!callback.empty())
                        CALL_S2S(callback, (int)ErrCode::api_db_error, callback_data);
                    return;
                }

                m_db->uno_game_players_save_role(uid, lept_value(std::move(data)),
                                                lept_value(std::move(summary)),
                                                [=](std::exception_ptr err)
                {
                    try
                    {
                        if (err)
                            std::rethrow_exception(err);
                    } catch (const std::exception& e)
                    {
                        std::cerr << "Unexpected exception when save role, uid: " << uid << std::endl;
                        std::cerr << e.what() << std::endl;

                        if (!callback.empty())
                            CALL_S2S(callback, (int)ErrCode::api_db_error, callback_data);
                    }

                    if (!callback.empty())
                    {
                        CALL_S2S(callback, (int)ErrCode::ok, callback_data);
                    }
                });
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
