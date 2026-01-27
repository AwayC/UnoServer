//
// Created by AWAY on 26-1-27.
//

#pragma once

#include "db.h"
#include "../core/db.h"
#include <mysqlx/xdevapi.h>

#include "leptjson.h"

#define POOL_SIZE 1

namespace uno
{
    using namespace mysqlx;
    class MysqlAgent : public IDataBase
    {
    public:
        MysqlAgent(Background* bg, const std::string& url) :
            IDataBase(bg),
            m_client(url, ClientOption::POOL_MAX_SIZE, POOL_SIZE)
        {

        }

        ~MysqlAgent() override
        {
            m_client.close();
        }

        static MysqlAgent* create(Background* bg, const std::string& path)
        {
            return new MysqlAgent(bg, path);
        }
        
        void connect(dbExcepCb cb) override;
        void connect() override;

        /**
         * users query
         * 有 callback 是异步ß，没有 callback 是阻塞
         */
    void users_has_username(const std::string& username,
                        dbResultCb<bool> cb) override;
    bool users_has_username(const std::string& username) override;

    void users_has_uid(int uid, dbResultCb<bool> cb) override;
    bool users_has_uid(int uid) override;

    void users_find_uid_by_name(const std::string& name,
                            dbResultCb<int> cb) override;
    int users_find_uid_by_name(const std::string& name) override;

    void users_create_user(const std::string& username,
                            const std::string& password,
                            const std::string& email,
                            dbExcepCb cb) override;
    void users_create_user(const std::string& username,
                            const std::string& password,
                            const std::string& email) override;

    void users_validate_password(const std::string& username,
                            const std::string& password,
                            dbResultCb<bool> cb) override;
    bool users_validate_password(const std::string& username,
                            const std::string& password) override;

    void users_query_user(const std::string& username,
                            dbResultCb<lept_value> cb) override;
    lept_value users_query_user(const std::string& username) override;

    /**
    * users update
    */
    void users_update_login_info(int uid, const std::string& login_ip,
                                    dbExcepCb cb) override;
    void users_update_login_info(int uid, const std::string& login_ip) override;

    void users_update_email(int uid, const std::string& email,
                                    dbExcepCb cb) override;
    void users_update_email(int uid, const std::string& email) override;

    void users_update_password(int uid, const std::string& password,
                                    dbExcepCb cb) override;
    void users_update_password(int uid, const std::string& password) override;

    /**
    * uno game query
    */
    void uno_game_players_has_role(int uid, dbResultCb<bool> cb) override; 
    bool uno_game_players_has_role(int uid) override;

    void uno_game_players_has_nickname(const std::string& name,
                                    dbResultCb<bool> cb) override;
    bool uno_game_players_has_nickname(const std::string& name) override;

    void uno_game_players_create_role(int uid, const std::string& nickname,
                                    dbExcepCb cb) override;
    void uno_game_players_create_role(int uid, const std::string& nickname) override;

    void uno_game_players_load_role(int uid, dbResultCb<lept_value> cb) override;
    lept_value uno_game_players_load_role(int uid) override;

    void uno_game_players_save_role(int uid, const lept_value& data,
                                const lept_value& summary,
                                dbExcepCb cb) override;
    void uno_game_players_save_role(int uid, const lept_value& data,
                                const lept_value& summary) override;

    void uno_game_players_load_summary(int uid, dbResultCb<lept_value> cb) override;
    lept_value uno_game_players_load_summary(int uid) override;


    private:
        // 连接池
        Client m_client;
    };
}

