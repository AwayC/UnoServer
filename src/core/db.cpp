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
                                            std::string& password,
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




};