//
// Created by AWAY on 25-10-14.
//

#pragma once

#include <iostream>
#include <cstring>
#include <SQLiteCpp/SQLiteCpp.h>
#include <leptjson.h>

namespace uno {
    class DataBase
    {
        //单例模式
        static DataBase* getIns();
        void connect();
        bool users_has_username(const std::string& username);
        bool users_has_uid(int uid);
        int users_find_uid_bu_name(const std::string& name);
        void users_create_user(const std::string& username,
                                    const std::string& password,
                                    const std::string& email);
        bool users_validate_password(const std::string& username,
                                    std::string& password);
        lept_value users_query_user(const std::string& username);

        /**
         * update
         */
        void users_update_login_info(int uid, const std::string& login_ip);
        void user_update_email(int uid, const std::string& email);
        void users_update_password(int uid, const std::string& password);


        void uno_game_players_has_role(int uid);
        bool uno_game_players_has_nickname(const std::string& name);
        void uno_game_players_create_role(int uid, const std::string& nickname);
        lept_value uno_game_players_load_role(int uid);
        void uno_game_players_save_role(int uid, const std::string& data,
                                        const std::string& summary);
        std::string uno_game_players_load_summary(int uid);


    private:
        SQLite::Database m_db;
        std::string m_dbPath;

        DataBase();
    };
};


