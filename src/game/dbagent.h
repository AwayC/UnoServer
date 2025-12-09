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
                std::cout << "dbagent load_role_req" << std::endl;
                m_db->uno_game_players_load_role(uid, [=, this](BackResult<lept_value> result)
                {
                    std::cout << "dbagent load_role_req callback: " << callback
                        << " uid: " << uid << " callback_data: " << callback_data << std::endl;
                    if (result.index() == 0)
                    {
                        lept_value& data = std::get<lept_value>(result);
                        if (data.get_type() == lept_type::null)
                        {
                            std::cout << "load_role_req call db not exists " << callback << std::endl;
                            CALL_S2S(callback, (int)ErrCode::db_not_exists,
                                                    "", lept_value::object_t(),
                                                    lept_value::object_t(),
                                                    callback_data);
                            return;
                        }

                        std::cout << "load_role_req call db success" << std::endl;
                        CALL_S2S(callback, (int)ErrCode::ok,
                                                data["nickname"],
                                                data["summary"].get_object(),
                                                data["data"].get_object(),
                                                callback_data);

                    } else
                    {
                        std::cerr << "Unexpected exception while loading role" << std::endl;
                        CALL_S2S(callback, (int)ErrCode::api_db_error,
                                            "", lept_value::object_t(),
                                            lept_value::object_t(),
                                            callback_data);
                    }

                });
            };

            std::function  create_role_req = [this](int uid,
                                const std::string& nick,
                                const std::string& callback,
                                int callback_data)
            {
                std::cout << "dbagent create_role_req" << std::endl;
                auto task = [=, this]()
                {
                    std::cout << "players_has_nickname: " << nick << std::endl;
                    bool exit = this->m_db->uno_game_players_has_nickname(nick);
                    std::cout << "players_has_nickname: " << exit << std::endl;
                    if (!exit)
                    {
                        this->m_db->uno_game_players_create_role(uid, nick);
                        std::cout << "uno_game_players_create_role success" << std::endl;
                    }

                    return exit;
                };

                m_db->get_bg()->submit<bool>(task, [=](BackResult<bool> result)
                {
                    if (result.index() == 0)
                    {
                        bool exit = std::get<bool>(result);
                        if (exit)
                        {
                            CALL_S2S(callback, (int)ErrCode::api_nick_in_use, callback_data);
                            return;
                        }
                    } else
                    {
                        std::cerr << "Unexpected exception while creating role" << std::endl;

                        CALL_S2S(callback, (int)ErrCode::api_db_error, callback_data);
                        return ;
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
                std::cout << "dbagent save_role_req" << std::endl;

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
                        return ;
                    }

                    if (!callback.empty())
                    {
                        CALL_S2S(callback, (int)ErrCode::ok, callback_data);
                    }
                });
            };

            Router::router().register_s2s("load_role_req", load_role_req);
            Router::router().register_s2s("create_role_req", create_role_req);
            Router::router().register_s2s("save_role_req", save_role_req);
        }
        ~DBagent() = default;

    private:
        DataBase* m_db;
    };

};
