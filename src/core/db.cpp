//
// Created by AWAY on 25-10-14.
//

#include "db.h"


namespace uno
{
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
        };

        m_bg->submit(task, cb);
    }

    void DataBase::users_has_username(const std::string& username,
                                        dbResultCb<bool> cb)
    {

        std::function<bool()> task = [this, username]()
        {
           bool ret = false;
           try
           {
               SQLite::Statement query(m_db, R"(SELECT * FROM users WHERE username = ?)");
               query.bind(1, username);
               ret = query.executeStep();
           } catch (std::exception& e)
           {
               std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
               throw;
           }
            return ret;
        };

        m_bg->submit(task, std::move(cb));
    }

    void DataBase::users_has_uid(int uid, dbResultCb<bool> cb)
    {
        std::function<bool()> task = [this, uid]()
        {
            bool ret = false;
            try
            {
                SQLite::Statement query(m_db, R"(SELECT * FROM users WHERE uid = ?)");
                query.bind(1, uid);
                ret = query.executeStep();
            } catch (std::exception& e)
            {
                std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
                throw;
            }
            return ret;
        };

        m_bg->submit(task, std::move(cb));
    }

    void DataBase::users_find_uid_by_name(const std::string& name,
                                            dbResultCb<int> cb)
    {
        std::function<int()> task = [this, name]()
        {
            int ret = -1;
            try
            {
                SQLite::Statement query(m_db, R"(SELECT uid FROM users WHERE username = ?)");
                query.bind(1, name);
                if (query.executeStep())
                {
                    ret = query.getColumn(0).getInt();
                }
            } catch (std::exception& e)
            {
                std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
                throw;
            }
            return ret;
        };

        m_bg->submit(task, std::move(cb));
    }

    void DataBase::users_create_user(const std::string& username,
                                    const std::string& password,
                                    const std::string& email,
                                    dbExcepCb cb)
    {
        auto task = [this, username, password, email]()
        {
            std::string salt = "salt";
            //todo: hash password and create salt

            try
            {
                SQLite::Statement query(m_db, R"(INSERT INTO users (username, password, salt, email, register_time) VALUES (?, ?, ?, ?, datetime('now')))");
                query.bind(1, username);
                query.bind(2, password);
                query.bind(3, salt);
                query.bind(4, email);
                query.exec();
            } catch (std::exception& e)
            {
                std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
                throw;
            }
        };

        m_bg->submit(task, std::move(cb));
    }

    void DataBase::users_validate_password(const std::string& username,
                                            const std::string& password,
                                            dbResultCb<bool> cb)
    {
        std::function<bool()> task = [this, username, password]()
        {
            bool ret = false;
            // todo: hash password and compare with stored password
            try
            {
                SQLite::Statement query(m_db, R"(SELECT password, salt FROM users WHERE username = ?)");
                query.bind(1, username);
                if (query.executeStep())
                {
                    std::string stored_password = query.getColumn(0).getString();
                    std::string stored_salt = query.getColumn(1).getString();
                    // todo: hash password with stored salt and compare with stored_password
                    ret = password == stored_password;
                }
            } catch (std::exception& e)
            {
                std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
                throw;
            }

            return ret;
        };

        m_bg->submit(task, std::move(cb));
    }


    void DataBase::users_query_user(const std::string& username,
                                dbResultCb<lept_value> cb)
    {
        std::function<lept_value()> task = [this, username]()
        {
           lept_value ret;
           try
           {
               SQLite::Statement query(m_db, R"(SELECT uid, email, register_time, last_login_time, last_login_ip FROM users WHERE username = ?)");
               query.bind(1, username);
               if (query.executeStep())
               {
                   ret = {
                       {"uid", query.getColumn(0).getInt()},
                       {"username", query.getColumn(1).getString()},
                       {"email", query.getColumn(2).getString()},
                       {"register_time", query.getColumn(3).getInt()},
                       {"last_login_time", query.getColumn(4).getInt()},
                       {"last_login_ip", query.getColumn(5).getString()}
                   };
               }
           } catch (std::exception& e)
           {
               std::cerr << "unexpected error when querying users: " << e.what() << std::endl;
               throw;
           }
           return ret;
        };

        m_bg->submit(task, std::move(cb));
    }

    void DataBase::users_update_login_info(int uid, const std::string& login_ip,
                                            dbExcepCb cb)
    {
        auto task = [this, uid, login_ip]()
        {
            try
            {
                SQLite::Statement query(m_db, R"(UPDATE users SET last_login_time = current_timestamp, last_login_ip = ? WHERE uid = ?)");
                query.bind(1, login_ip);

            } catch (std::exception& e)
            {
                std::cerr << "unexpected error when update users: " << e.what() << std::endl;
                throw;
            }
        };

        m_bg->submit(task, std::move(cb));
    }

    void DataBase::user_update_email(int uid, const std::string& email,
                                    dbExcepCb cb)
    {
        auto task = [this, uid, email]()
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
        };

        m_bg->submit(task, std::move(cb));
    }

    void DataBase::uno_game_players_has_role(int uid, dbResultCb<bool> cb)
    {
        std::function<bool()> task = [this, uid]()
        {
            bool ret = false;

            try
            {
                SQLite::Statement query(m_db, R"(SELECT uid FROM uno_game_players WHERE uid = ?)");
                query.bind(1, uid);
                ret = query.executeStep();
            } catch (std::exception& e)
            {
                std::cerr << "unexpected error when update users: " << e.what() << std::endl;
                throw;
            }

            return ret;
        };

        m_bg->submit(task, std::move(cb));
    }

    void DataBase::uno_game_players_has_nickname(const std::string& name,
                                                dbResultCb<bool> cb)
    {
        std::function<bool()> task = [this, name]()
        {
            bool ret = false;

           try
           {
               SQLite::Statement query(m_db, R"(SELECT uid FROM uno_game_players WHERE nickname = ?)");
               query.bind(1, name);
               ret = query.executeStep();
           } catch (std::exception& e)
           {
               std::cerr << "unexpected error when update users: " << e.what() << std::endl;
               throw;
           }
            return ret;
        };

        m_bg->submit(task, std::move(cb));
    }

    void DataBase::uno_game_players_create_role(int uid, const std::string& nickname, dbExcepCb cb)
    {
        auto task = [this, uid, nickname]()
        {
            try
            {
                SQLite::Statement query(m_db, R"(INSERT INTO uno_game_players VALUES (?, ?, datetime('now'), {}, {})");
                query.bind(1, uid);
                query.bind(1, nickname);
            } catch (std::exception& e)
            {
                std::cerr << "unexpected error when update users: " << e.what() << std::endl;
                throw;
            }
        };

        m_bg->submit(task, std::move(cb));
    }





};

