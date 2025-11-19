//
// Created by AWAY on 25-10-14.
//

#pragma once

#include <iostream>
#include <cstring>
#include <SQLiteCpp/SQLiteCpp.h>
#include <leptjson.h>
#include "background.h"
#include <mutex>

namespace uno {

    template<typename T>
    using dbResultCb = std::function<void(BackResult<T>)>;
    using dbExcepCb = std::function<void(std::exception_ptr)>;

    class DataBase : public std::enable_shared_from_this<DataBase>
    {
    public:
        DataBase(Background* bg, std::string path) :
            m_bg(bg),
            m_db(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
        {
            assert(bg);
        }

        ~DataBase() = default;
        //工厂模式
        static DataBase* create(Background* bg, std::string path);

        Background* get_bg() const
        {
            return m_bg;
        }

        void connect(dbExcepCb cb);
        void connect();

        /**
         * users query
         * 有 callback 是异步ß，没有 callback 是阻塞
         */
        void users_has_username(const std::string& username,
                                dbResultCb<bool> cb);
        bool users_has_username(const std::string& username);

        void users_has_uid(int uid, dbResultCb<bool> cb);
        bool users_has_uid(int uid);

        void users_find_uid_by_name(const std::string& name,
                                    dbResultCb<int> cb);
        int users_find_uid_by_name(const std::string& name);

        void users_create_user(const std::string& username,
                                    const std::string& password,
                                    const std::string& email,
                                    dbExcepCb cb);
        void users_create_user(const std::string& username,
                                    const std::string& password,
                                    const std::string& email);

        void users_validate_password(const std::string& username,
                                    const std::string& password,
                                    dbResultCb<bool> cb);
        bool users_validate_password(const std::string& username,
                                    const std::string& password);

        void users_query_user(const std::string& username,
                                    dbResultCb<lept_value> cb);
        lept_value users_query_user(const std::string& username);

        /**
         * users update
         */
        void users_update_login_info(int uid, const std::string& login_ip,
                                            dbExcepCb cb);
        void users_update_login_info(int uid, const std::string& login_ip);

        void users_update_email(int uid, const std::string& email,
                                            dbExcepCb cb);
        void users_update_email(int uid, const std::string& email);

        void users_update_password(int uid, const std::string& password,
                                            dbExcepCb cb);
        void users_update_password(int uid, const std::string& password);

        /**
         * uno game query
         */
        void uno_game_players_has_role(int uid, dbResultCb<bool> cb);
        bool uno_game_players_has_role(int uid);

        void uno_game_players_has_nickname(const std::string& name,
                                            dbResultCb<bool> cb);
        bool uno_game_players_has_nickname(const std::string& name);

        void uno_game_players_create_role(int uid, const std::string& nickname,
                                            dbExcepCb cb);
        void uno_game_players_create_role(int uid, const std::string& nickname);

        void uno_game_players_load_role(int uid, dbResultCb<lept_value> cb);
        lept_value uno_game_players_load_role(int uid);

        void uno_game_players_save_role(int uid, const lept_value& data,
                                        const lept_value& summary,
                                        dbExcepCb cb);
        void uno_game_players_save_role(int uid, const lept_value& data,
                                        const lept_value& summary);

        void uno_game_players_load_summary(int uid, dbResultCb<lept_value> cb);
        lept_value uno_game_players_load_summary(int uid);



    private:
        SQLite::Database m_db;
        std::string m_dbPath;
        std::mutex m_mutex;
        Background* m_bg;



    };

};


