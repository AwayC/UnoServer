//
// Created by AWAY on 25-10-14.
//

#include "mysql.h"
#include "../util/ssl.h"
#include <algorithm>
#include <functional>
#include <mysqlx/xdevapi.h>
#include <sstream>

namespace uno
{
    using namespace mysqlx;

    std::string trim_and_lower(const std::string& str)
    {
        std::string result = str;
        result.erase(0, result.find_first_not_of(" \t\n\r"));
        result.erase(result.find_last_not_of(" \t\n\r") + 1);
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    std::string valToString(const mysqlx::Value& val) {
        std::stringstream ss;
        ss << val;
        return ss.str();
    }

    void MysqlAgent::connect(dbExcepCb cb)
    {
        std::cout << "Connect to DB" << std::endl;
        auto task = [this]()
        {
            connect();
        };

        m_bg->submit(task, std::move(cb));
    }

    void MysqlAgent::connect()
    {
        std::cout << "BG Connect to DB" << std::endl;

        Session sess = m_client.getSession();
        try {
            std::cout << "test create table" << std::endl;

            sess.sql(R"(
                CREATE TABLE IF NOT EXISTS users (
                    uid INT AUTO_INCREMENT PRIMARY KEY,
                    username VARCHAR(64) NOT NULL,
                    password VARCHAR(128) NOT NULL,
                    salt VARCHAR(32) NOT NULL,
                    email VARCHAR(128) NOT NULL,
                    register_time DATETIME DEFAULT CURRENT_TIMESTAMP,
                    last_login_time DATETIME,
                    last_login_ip VARCHAR(64),
                    UNIQUE (username)
                ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
            )").execute();

            sess.sql(R"(
                CREATE TABLE IF NOT EXISTS uno_game_players (
                    uid INT PRIMARY KEY NOT NULL,
                    nickname VARCHAR(64) NOT NULL,
                    creation_time DATETIME DEFAULT CURRENT_TIMESTAMP,
                    summary JSON NOT NULL,
                    data JSON NOT NULL,
                    UNIQUE (nickname),
                    CONSTRAINT fk_users_uid
                        FOREIGN KEY (uid) REFERENCES users(uid)
                        ON DELETE CASCADE
                ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
            )").execute();

        } catch (const Error& e)
        {
            std::cerr << "Unexpected error when init db: " << e << std::endl;
            throw;
        } catch (std::exception& e)
        {
            std::cerr << "Unexpected error when init db: " << e.what() << std::endl;
            throw;
        }
    }

    void MysqlAgent::users_has_username(const std::string& username,
                                        dbResultCb<bool> cb)
    {
        std::cout << "db users_has_username" << std::endl;
        std::function<bool()> task = [this, username]()
        {
            return users_has_username(username);
        };

        m_bg->submit<bool>(task, std::move(cb));
    }

    bool MysqlAgent::users_has_username(const std::string& username)
    {
        std::string name = trim_and_lower(username);
        Session sess = m_client.getSession();

        try
        {
            auto result = sess.sql(R"(SELECT uid FROM users WHERE username = ?)")
                                        .bind(name)
                                        .execute();
            return (bool)result.fetchOne();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
            throw;
        }
    }

    void MysqlAgent::users_has_uid(int uid, dbResultCb<bool> cb)
    {
        std::function<bool()> task = [this, uid]()
        {
            return users_has_uid(uid);
        };

        m_bg->submit<bool>(task, std::move(cb));
    }

    bool MysqlAgent::users_has_uid(int uid)
    {
        Session sess = m_client.getSession();

        try
        {
            auto result = sess.sql(R"(SELECT uid FROM users WHERE uid = ?)")
                            .bind(uid)
                            .execute();
            return (bool)result.fetchOne();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
            throw;
        }
    }

    void MysqlAgent::users_find_uid_by_name(const std::string& name,
                                            dbResultCb<int> cb)
    {
        std::function<int()> task = [this, name]()
        {
            return users_find_uid_by_name(name);
        };

        m_bg->submit<int>(task, std::move(cb));
    }

    int MysqlAgent::users_find_uid_by_name(const std::string& name)
    {
        std::string username = trim_and_lower(name);
        Session sess = m_client.getSession();

        try
        {
            auto result = sess.sql(R"(SELECT uid FROM users WHERE username = ?)")
                            .bind(username)
                            .execute();
            Row row = result.fetchOne();
            if (row)
                return row[0].get<int>();
            else
                return 0;
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
            throw;
        }
    }

    void MysqlAgent::users_create_user(const std::string& username,
                                    const std::string& password,
                                    const std::string& email,
                                    dbExcepCb cb)
    {
        auto task = [this, username, password, email]()
        {
            users_create_user(username, password, email);
        };

        m_bg->submit(task, std::move(cb));
    }

    void MysqlAgent::users_create_user(const std::string& username,
                                    const std::string& password,
                                    const std::string& email)
    {
        std::string salt = "salt";

        std::string name = trim_and_lower(username);
        assert(!name.empty());

        salt = generate_salt();
        std::string hashed = md5(salt + password);
        
        Session sess = m_client.getSession();

        try
        {
            sess.sql( R"(INSERT INTO users (username, password, salt, email, register_time) VALUES (?, ?, ?, ?, NOW()))")
                .bind(name, hashed, salt, email)
                .execute();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
            throw;
        }
    }

    void MysqlAgent::users_validate_password(const std::string& username,
                                            const std::string& password,
                                            dbResultCb<bool> cb)
    {
        std::function<bool()> task = [this, username, password]()
        {
            return users_validate_password(username, password);
        };

        m_bg->submit<bool>(task, std::move(cb));
    }



    bool MysqlAgent::users_validate_password(const std::string& username,
                                            const std::string& password)
    {
        std::string name = trim_and_lower(username);
        std::string hashed, salt;
        Session sess = m_client.getSession();

        try
        {
            auto result = sess.sql(R"(SELECT password, salt FROM users WHERE username = ?)")
                            .bind(name)
                            .execute();
            Row row = result.fetchOne();
            if (row)
            {
                hashed = row[0].get<std::string>();
                salt = row[1].get<std::string>();
            } else {
                return false;
            }
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
            throw;
        }

        std::string validate = md5(salt + password);
        return validate == hashed;
    }



    void MysqlAgent::users_query_user(const std::string& username,
                                dbResultCb<lept_value> cb)
    {
        std::function<lept_value()> task = [this, username]()
        {
            return users_query_user(username);
        };

        m_bg->submit<lept_value>(task, std::move(cb));
    }

    lept_value MysqlAgent::users_query_user(const std::string& username)
    {
        std::string name = trim_and_lower(username);
        Session sess = m_client.getSession();

        try
        {
            auto result = sess.sql(R"(SELECT uid, email, register_time, last_login_time, last_login_ip FROM users WHERE username = ?)")
                .bind(name)
                .execute();
            Row row = result.fetchOne();
            if (row)
            {
               return {
                    {"uid", row[0].get<int>()},
                    {"username", name},
                    {"email", row[1].get<std::string>()},
                    {"register_time", row[2].get<std::string>()}, 
                    {"last_login_time", row[3].isNull() ? "" : row[3].get<std::string>()},
                    {"last_login_ip", row[4].isNull() ? "" : row[4].get<std::string>()}
                };
            }
            return {};
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
            throw;
        }
    }

    void MysqlAgent::users_update_login_info(int uid, const std::string& login_ip,
                                            dbExcepCb cb)
    {
        auto task = [this, uid, login_ip]()
        {
            users_update_login_info(uid, login_ip);
        };

        m_bg->submit(task, std::move(cb));
    }

    void MysqlAgent::users_update_login_info(int uid, const std::string& login_ip)
    {
        Session sess = m_client.getSession();

        try
        {
            sess.sql(R"(UPDATE users SET last_login_time = NOW(), last_login_ip = ? WHERE uid = ?)")
                .bind(login_ip, uid)
                .execute();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when update users: " << e.what() << std::endl;
            throw;
        }
    }

    void MysqlAgent::users_update_email(int uid, const std::string& email,
                                    dbExcepCb cb)
    {
        auto task = [this, uid, email]()
        {
            users_update_email(uid, email);
        };

        m_bg->submit(task, std::move(cb));
    }

    void MysqlAgent::users_update_email(int uid, const std::string& email)
    {
        Session sess = m_client.getSession();
        try
        {
            sess.sql(R"(UPDATE users SET email = ? WHERE uid = ?)")
                .bind(email, uid)
                .execute();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when update users: " << e.what() << std::endl;
            throw;
        }
    }

    void MysqlAgent::uno_game_players_has_role(int uid, dbResultCb<bool> cb)
    {
        std::function<bool()> task = [this, uid]()
        {
            return uno_game_players_has_role(uid);
        };

        m_bg->submit<bool>(task, std::move(cb));
    }

    bool MysqlAgent::uno_game_players_has_role(int uid)
    {
        Session sess = m_client.getSession();
        try
        {
            auto result = sess.sql(R"(SELECT uid FROM uno_game_players WHERE uid = ?)")
                            .bind(uid)
                            .execute();
            return (bool)result.fetchOne();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when update users: " << e.what() << std::endl;
            throw;
        }
    }
    
    void MysqlAgent::uno_game_players_has_nickname(const std::string& name,
                                                dbResultCb<bool> cb)
    {
        std::function<bool()> task = [this, name]()
        {
            return uno_game_players_has_nickname(name);
        };

        m_bg->submit<bool>(task, std::move(cb));
    }

    bool MysqlAgent::uno_game_players_has_nickname(const std::string& name)
    {
        Session sess = m_client.getSession();
        try
        {
            auto result = sess.sql(R"(SELECT uid FROM uno_game_players WHERE nickname = ?)")
                            .bind(name)
                            .execute();
            return (bool)result.fetchOne();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when update users: " << e.what() << std::endl;
            throw;
        }
    }

    void MysqlAgent::uno_game_players_create_role(int uid, const std::string& nickname, dbExcepCb cb)
    {
        auto task = [this, uid, nickname]()
        {
            uno_game_players_create_role(uid, nickname);
        };

        m_bg->submit(task, std::move(cb));
    }

    void MysqlAgent::uno_game_players_create_role(int uid, const std::string& nickname)
    {
        Session sess = m_client.getSession();
        try
        {
            sess.sql(R"(INSERT INTO uno_game_players (uid, nickname, creation_time, summary, data) VALUES (?, ?, NOW(), '{}', '{}'))")
                .bind(uid, nickname)
                .execute();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when update users: " << e.what() << std::endl;
            throw;
        }
    }

    void MysqlAgent::uno_game_players_load_role(int uid, dbResultCb<lept_value> cb)
    {
        std::function<lept_value()> task = [this, uid]()
        {
            return uno_game_players_load_role(uid);
        };

        m_bg->submit<lept_value>(task, std::move(cb));
    }

    lept_value MysqlAgent::uno_game_players_load_role(int uid)
    {
        Session sess = m_client.getSession();
        try
        {
            auto result = sess.sql(R"(SELECT nickname, creation_time, summary, data FROM uno_game_players WHERE uid = ?)")
                            .bind(uid)
                            .execute();
            Row row = result.fetchOne();

            if (row)
            {
                lept_value summary, data;

                summary.parse(valToString(row[2]));
                data.parse(valToString(row[3]));

                assert(summary.is<lept_value::object_t>());
                assert(data.is<lept_value::object_t>());

                return {
                            {"uid", uid},
                            {"nickname", row[0].get<std::string>()},
                            {"creation_time", row[1].get<std::string>()},
                            {"summary", summary},
                            {"data", data},
                        };
            }

            return {};

        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when update users: " << e.what() << std::endl;
            throw;
        }
    }

    void MysqlAgent::uno_game_players_save_role(int uid, const lept_value& data,
                                                const lept_value& summary,
                                                dbExcepCb cb)
    {
        auto task = [this, uid, data, summary]()
        {
            uno_game_players_save_role(uid, data, summary);
        };

        m_bg->submit(task, std::move(cb));
    }

    void MysqlAgent::uno_game_players_save_role(int uid, const lept_value& data,
                                        const lept_value& summary)
    {
        Session sess = m_client.getSession();
        try
        {
            sess.sql(R"(UPDATE uno_game_players SET data = ?, summary = ? WHERE uid = ?)")
                .bind(data.stringify(), summary.stringify(), uid)
                .execute();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when update users: " << e.what() << std::endl;
            throw;
        }
    }

    void MysqlAgent::uno_game_players_load_summary(int uid, dbResultCb<lept_value> cb)
    {
        std::function<lept_value()> task = [this, uid]()
        {
            return uno_game_players_load_summary(uid);
        };

        m_bg->submit<lept_value>(task, std::move(cb));
    }

    lept_value MysqlAgent::uno_game_players_load_summary(int uid)
    {
        lept_value ret;
        Session sess = m_client.getSession();
        try
        {
            auto result = sess.sql(R"(SELECT summary FROM uno_game_players WHERE uid = ?)")
                            .bind(uid)
                            .execute();
            Row row = result.fetchOne();
            if (row)
            {
                ret.parse(valToString(row[0]));
            }
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when load users: " << e.what() << std::endl;
            throw;
        }

        return ret;
    }

    void MysqlAgent::users_update_password(int uid, const std::string& password,
                                            dbExcepCb cb)
    {
        auto task = [this, uid, password]()
        {
            users_update_password(uid, password);
        };

        m_bg->submit(task, std::move(cb));
    }

    void MysqlAgent::users_update_password(int uid, const std::string& password)
    {
        std::string salt = generate_salt();
        std::string hashed = md5(salt + password);

        Session sess = m_client.getSession();
        try
        {
            sess.sql(R"(UPDATE users SET password = ?, salt = ? WHERE uid = ?)")
                .bind(hashed, salt, uid)
                .execute();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when update users: " << e.what() << std::endl;
            throw;
        }
    }



};