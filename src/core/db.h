//
// Created by AWAY on 25-10-14.
//

#pragma once

#include <iostream>
#include <cstring>
#include <leptjson.h>
#include "background.h"
#include <mutex>

namespace uno {

    template<typename T>
    using dbResultCb = std::function<void(BackResult<T>)>;
    using dbExcepCb = std::function<void(std::exception_ptr)>;

    // 抽象数据库接口
    class IDataBase
    {
    public:
        IDataBase(Background* bg)
        {
            m_bg = bg;
        }

        virtual ~IDataBase() = default;

        Background* get_bg() const
        {
            return m_bg;
        }

        virtual void connect(dbExcepCb cb) = 0;
        virtual void connect() = 0;

        /**
         * users query
         * 有 callback 是异步ß，没有 callback 是阻塞
         */
        virtual void users_has_username(const std::string& username,
                                dbResultCb<bool> cb) = 0;
        virtual bool users_has_username(const std::string& username) = 0;

        virtual void users_has_uid(int uid, dbResultCb<bool> cb) = 0;
        virtual bool users_has_uid(int uid) = 0;

        virtual void users_find_uid_by_name(const std::string& name,
                                    dbResultCb<int> cb) = 0;
        virtual int users_find_uid_by_name(const std::string& name) = 0;

        virtual void users_create_user(const std::string& username,
                                    const std::string& password,
                                    const std::string& email,
                                    dbExcepCb cb) = 0;
        virtual void users_create_user(const std::string& username,
                                    const std::string& password,
                                    const std::string& email) = 0;

        virtual void users_validate_password(const std::string& username,
                                    const std::string& password,
                                    dbResultCb<bool> cb) = 0;
        virtual bool users_validate_password(const std::string& username,
                                    const std::string& password) = 0;

        virtual void users_query_user(const std::string& username,
                                    dbResultCb<lept_value> cb) = 0;
        virtual lept_value users_query_user(const std::string& username) = 0;

        /**
         * users update
         */
        virtual void users_update_login_info(int uid, const std::string& login_ip,
                                            dbExcepCb cb) = 0;
        virtual void users_update_login_info(int uid, const std::string& login_ip) = 0;

        virtual void users_update_email(int uid, const std::string& email,
                                            dbExcepCb cb) = 0;
        virtual void users_update_email(int uid, const std::string& email) = 0;

        virtual void users_update_password(int uid, const std::string& password,
                                            dbExcepCb cb) = 0;
        virtual void users_update_password(int uid, const std::string& password) = 0;

        /**
         * uno game query
         */
        virtual void uno_game_players_has_role(int uid, dbResultCb<bool> cb) = 0; 
        virtual bool uno_game_players_has_role(int uid) = 0;

        virtual void uno_game_players_has_nickname(const std::string& name,
                                            dbResultCb<bool> cb) = 0;
        virtual bool uno_game_players_has_nickname(const std::string& name) = 0;

        virtual void uno_game_players_create_role(int uid, const std::string& nickname,
                                            dbExcepCb cb) = 0;
        virtual void uno_game_players_create_role(int uid, const std::string& nickname) = 0;

        virtual void uno_game_players_load_role(int uid, dbResultCb<lept_value> cb) = 0;
        virtual lept_value uno_game_players_load_role(int uid) = 0;

        virtual void uno_game_players_save_role(int uid, const lept_value& data,
                                        const lept_value& summary,
                                        dbExcepCb cb) = 0;
        virtual void uno_game_players_save_role(int uid, const lept_value& data,
                                        const lept_value& summary) = 0;

        virtual void uno_game_players_load_summary(int uid, dbResultCb<lept_value> cb) = 0;
        virtual lept_value uno_game_players_load_summary(int uid) = 0;


    protected:
        Background* m_bg;

    };

};


