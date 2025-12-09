//
// Created by AWAY on 25-10-14.
//

#include "db.h"
#include "../util/ssl.h"

namespace uno
{
    std::string trim_and_lower(const std::string& str)
    {
        std::string result = str;
        result.erase(0, result.find_first_not_of(" \t\n\r"));
        result.erase(result.find_last_not_of(" \t\n\r") + 1);
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    DataBase* DataBase::create(Background* bg, std::string path)
    {
        DataBase* db = new DataBase(bg, path);
        return db;
    }

    void DataBase::connect(dbExcepCb cb)
    {
        std::cout << "Connect to DB" << std::endl;
        auto task = [this]()
        {
            connect();
        };

        m_bg->submit(task, std::move(cb));
    }

    void DataBase::connect()
    {
        std::cout << "BG Connect to DB" << std::endl;
        try {

            m_db.exec("PRAGMA foreign_keys = ON;");

            SQLite::Transaction transaction(m_db);
            m_db.exec(R"(CREATE TABLE IF NOT EXISTS users (
                    uid INTEGER PRIMARY KEY,
                    username TEXT NOT NULL,
                    password TEXT NOT NULL,
                    salt TEXT NOT NULL,
                    email TEXT NOT NULL,
                    register_time DATETIME,
                    last_login_time DATETIME,
                    last_login_ip TEXT,
                    UNIQUE (username)
                    );)"
            );
            m_db.exec(R"(CREATE TABLE IF NOT EXISTS uno_game_players (
                uid INTEGER PRIMARY KEY NOT NULL REFERENCES users(uid),
                nickname TEXT NOT NULL,
                creation_time DATETIME NOT NULL,
                summary JSON NOT NULL,
                data JSON NOT NULL,
                UNIQUE (nickname)
                );)"
            );

            transaction.commit();

        } catch (std::exception& e)
        {
            std::cerr << "Unexpected error when init db: " << e.what() << std::endl;
            throw;
        }
    }

    void DataBase::users_has_username(const std::string& username,
                                        dbResultCb<bool> cb)
    {
        std::cout << "db users_has_username" << std::endl;
        std::string name = trim_and_lower(username);
        std::function<bool()> task = [this, username]()
        {
            return users_has_username(username);
        };

        m_bg->submit<bool>(task, std::move(cb));
    }

    bool DataBase::users_has_username(const std::string& username)
    {
        std::string name = trim_and_lower(username);
        try
        {
            SQLite::Statement query(m_db, R"(SELECT uid FROM users WHERE username = ?)");
            query.bind(1, name);
            return query.executeStep();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
            throw;
        }
    }

    void DataBase::users_has_uid(int uid, dbResultCb<bool> cb)
    {
        std::function<bool()> task = [this, uid]()
        {
            return users_has_uid(uid);
        };

        m_bg->submit<bool>(task, std::move(cb));
    }

    bool DataBase::users_has_uid(int uid)
    {
        try
        {
            SQLite::Statement query(m_db, R"(SELECT uid FROM users WHERE uid = ?)");
            query.bind(1, uid);
            return query.executeStep();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
            throw;
        }
    }

    void DataBase::users_find_uid_by_name(const std::string& name,
                                            dbResultCb<int> cb)
    {
        std::function<int()> task = [this, name]()
        {
            return users_find_uid_by_name(name);
        };

        m_bg->submit<int>(task, std::move(cb));
    }

    int DataBase::users_find_uid_by_name(const std::string& name)
    {
        std::string username = trim_and_lower(name);
        try
        {
            SQLite::Statement query(m_db, R"(SELECT uid FROM users WHERE username = ?)");
            query.bind(1, username);
            return query.executeStep();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
            throw;
        }
    }

    void DataBase::users_create_user(const std::string& username,
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

    void DataBase::users_create_user(const std::string& username,
                                    const std::string& password,
                                    const std::string& email)
    {
        std::string salt = "salt";

        std::string name = trim_and_lower(username);
        assert(!name.empty());

        salt = generate_salt();
        std::string hashed = md5(salt + password);

        try
        {
            SQLite::Statement query(m_db, R"(INSERT INTO users (username, password, salt, email, register_time) VALUES (?, ?, ?, ?, current_timestamp))");
            query.bind(1, name);
            query.bind(2, hashed);
            query.bind(3, salt);
            query.bind(4, email);
            query.exec();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
            throw;
        }
    }

    void DataBase::users_validate_password(const std::string& username,
                                            const std::string& password,
                                            dbResultCb<bool> cb)
    {
        std::function<bool()> task = [this, username, password]()
        {
            return users_validate_password(username, password);
        };

        m_bg->submit<bool>(task, std::move(cb));
    }



    bool DataBase::users_validate_password(const std::string& username,
                                            const std::string& password)
    {
        std::string name = trim_and_lower(username);
        std::string hashed, salt;

        try
        {
            SQLite::Statement query(m_db, R"(SELECT password, salt FROM users WHERE username = ?)");
            query.bind(1, username);
            if (query.executeStep())
            {
                hashed = query.getColumn(0).getString();
                salt = query.getColumn(1).getString();
                // todo: hash password with stored salt and compare with stored_password
            }
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
            throw;
        }

        std::string validate = md5(salt + password);
        return validate == hashed;
    }



    void DataBase::users_query_user(const std::string& username,
                                dbResultCb<lept_value> cb)
    {
        std::function<lept_value()> task = [this, username]()
        {
            return users_query_user(username);
        };

        m_bg->submit<lept_value>(task, std::move(cb));
    }

    lept_value DataBase::users_query_user(const std::string& username)
    {
        std::string name = trim_and_lower(username);
        try
        {
            SQLite::Statement query(m_db, R"(SELECT uid, email, register_time, last_login_time, last_login_ip FROM users WHERE username = ?)");
            query.bind(1, name);
            if (query.executeStep())
            {
               return {
                    {"uid", query.getColumn(0).getInt()},
                    {"username", name},
                    {"email", query.getColumn(1).getString()},
                    {"register_time", query.getColumn(2).getInt()},
                    {"last_login_time", query.getColumn(3).getInt()},
                    {"last_login_ip", query.getColumn(4).getString()}
                };
            }
            return {};
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
            throw;
        }
    }

    void DataBase::users_update_login_info(int uid, const std::string& login_ip,
                                            dbExcepCb cb)
    {
        auto task = [this, uid, login_ip]()
        {
            users_update_login_info(uid, login_ip);
        };

        m_bg->submit(task, std::move(cb));
    }

    void DataBase::users_update_login_info(int uid, const std::string& login_ip)
    {
        try
        {
            SQLite::Statement query(m_db, R"(UPDATE users SET last_login_time = current_timestamp, last_login_ip = ? WHERE uid = ?)");
            query.bind(1, login_ip);
            query.bind(2, uid);
            query.exec();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when update users: " << e.what() << std::endl;
            throw;
        }
    }

    void DataBase::users_update_email(int uid, const std::string& email,
                                    dbExcepCb cb)
    {
        auto task = [this, uid, email]()
        {
            users_update_email(uid, email);
        };

        m_bg->submit(task, std::move(cb));
    }

    void DataBase::users_update_email(int uid, const std::string& email)
    {
        try
        {
            SQLite::Statement query(m_db, R"(UPDATE users SET email = ? WHERE uid = ?)");
            query.bind(1, email);
            query.bind(2, uid);
            query.exec();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when update users: " << e.what() << std::endl;
            throw;
        }
    }

    void DataBase::uno_game_players_has_role(int uid, dbResultCb<bool> cb)
    {
        std::function<bool()> task = [this, uid]()
        {
            return uno_game_players_has_role(uid);
        };

        m_bg->submit<bool>(task, std::move(cb));
    }

    bool DataBase::uno_game_players_has_role(int uid)
    {
        try
        {
            SQLite::Statement query(m_db, R"(SELECT uid FROM uno_game_players WHERE uid = ?)");
            query.bind(1, uid);
            return query.executeStep();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when update users: " << e.what() << std::endl;
            throw;
        }
    }
    \
    void DataBase::uno_game_players_has_nickname(const std::string& name,
                                                dbResultCb<bool> cb)
    {
        std::function<bool()> task = [this, name]()
        {
            return uno_game_players_has_nickname(name);
        };

        m_bg->submit<bool>(task, std::move(cb));
    }

    bool DataBase::uno_game_players_has_nickname(const std::string& name)
    {
        try
        {
            SQLite::Statement query(m_db, R"(SELECT uid FROM uno_game_players WHERE nickname = ?)");
            query.bind(1, name);
            return query.executeStep();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when update users: " << e.what() << std::endl;
            throw;
        }
    }

    void DataBase::uno_game_players_create_role(int uid, const std::string& nickname, dbExcepCb cb)
    {
        auto task = [this, uid, nickname]()
        {
            uno_game_players_create_role(uid, nickname);
        };

        m_bg->submit(task, std::move(cb));
    }

    void DataBase::uno_game_players_create_role(int uid, const std::string& nickname)
    {
        try
        {
            SQLite::Statement query(m_db, R"(INSERT INTO uno_game_players (uid, nickname, creation_time, summary, data) VALUES (?, ?, current_timestamp, '{}', '{}'))");
            query.bind(1, uid);
            query.bind(2, nickname);
            query.exec();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when update users: " << e.what() << std::endl;
            throw;
        }
    }

    void DataBase::uno_game_players_load_role(int uid, dbResultCb<lept_value> cb)
    {
        std::function<lept_value()> task = [this, uid]()
        {
            return uno_game_players_load_role(uid);
        };

        m_bg->submit<lept_value>(task, std::move(cb));
    }

    lept_value DataBase::uno_game_players_load_role(int uid)
    {
        try
        {
            SQLite::Statement query(m_db, R"(SELECT nickname, creation_time, summary, data FROM uno_game_players WHERE uid = ?)");
            query.bind(1, uid);
            if (query.executeStep())
            {
                lept_value summary, data;
                summary.parse(query.getColumn(2).getString());
                data.parse(query.getColumn(3).getString());

                assert(summary.is<lept_value::object_t>());
                assert(data.is<lept_value::object_t>());

                return {
                            {"uid", uid},
                            {"nickname", query.getColumn(0).getString()},
                            {"creation_time", query.getColumn(1).getInt()},
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

    void DataBase::uno_game_players_save_role(int uid, const lept_value& data,
                                                const lept_value& summary,
                                                dbExcepCb cb)
    {
        auto task = [this, uid, data, summary]()
        {
            uno_game_players_save_role(uid, data, summary);
        };

        m_bg->submit(task, std::move(cb));
    }

    void DataBase::uno_game_players_save_role(int uid, const lept_value& data,
                                        const lept_value& summary)
    {
        try
        {
            SQLite::Statement query(m_db, R"(UPDATE uno_game_players SET data = ?, summary = ? WHERE uid = ?)");
            query.bind(3, uid);
            query.bind(1, data.stringify());
            query.bind(2, summary.stringify());
            query.exec();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when update users: " << e.what() << std::endl;
            throw;
        }
    }

    void DataBase::uno_game_players_load_summary(int uid, dbResultCb<lept_value> cb)
    {
        std::function<lept_value()> task = [this, uid]()
        {
            return uno_game_players_load_summary(uid);
        };

        m_bg->submit<lept_value>(task, std::move(cb));
    }

    lept_value DataBase::uno_game_players_load_summary(int uid)
    {
        lept_value ret;
        try
        {
            SQLite::Statement query(m_db, R"(SELECT summary FROM uno_game_players WHERE uid = ?)");
            query.bind(1, uid);
            if (query.executeStep())
            {
                ret.parse(query.getColumn(0).getString());
            }
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when load users: " << e.what() << std::endl;
            throw;
        }

        return ret;
    }

    void DataBase::users_update_password(int uid, const std::string& password,
                                            dbExcepCb cb)
    {
        auto task = [this, uid, password]()
        {
            users_update_password(uid, password);
        };

        m_bg->submit(task, std::move(cb));
    }

    void DataBase::users_update_password(int uid, const std::string& password)
    {
        std::string salt = generate_salt();
        std::string hashed = md5(salt + password);

        try
        {
            SQLite::Statement query(m_db, R"(UPDATE users SET password = ?, salt = ? WHERE uid = ?)");
            query.bind(1, hashed);
            query.bind(2, salt);
            query.bind(3, uid);
            query.exec();
        } catch (std::exception& e)
        {
            std::cerr << "unexpected error when update users: " << e.what() << std::endl;
            throw;
        }
    }



};

