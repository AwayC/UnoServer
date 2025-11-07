//
// Created by AWAY on 25-10-29.
//
#include <iostream>
#include <exception>

#include "sqlite3.h"
#include "SQLiteCpp/SQLiteCpp.h"

#define DB_PATH "test_sqlite.db3"

void init_tables(SQLite::Database& db)
{
    std::cout << "test connect (init tables)" << std::endl;
    try {
        SQLite::Transaction transaction(db);
        db.exec(R"(CREATE TABLE IF NOT EXISTS users (
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
        db.exec(R"(CREATE TABLE IF NOT EXISTS uno_game_players (
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

void test_insert(SQLite::Database& db)
{
    std::cout << "test insert" << std::endl;
    // create user
    std::string username = "away";
    std::string password = "123456";
    std::string email = "away@example.com";
    std::string salt = "123456";

    bool has_user = false;

    try
    {
        SQLite::Statement query(db, R"(SELECT uid FROM users WHERE username = ?;)");
        query.bind(1, username);
        has_user = query.executeStep();
    } catch (std::exception& e)
    {
        std::cerr << "Unexpected error when query user: " << e.what() << std::endl;
        throw;
    }

    if (has_user)
    {
        std::cout << "user exists" << std::endl;
        return ;
    }

    try
    {
        SQLite::Statement query(db,
            R"(
            INSERT INTO users (username, password, salt, email, register_time)
            VALUES (?, ?, ?, ?, DATETIME('now'));
            );)"
        );
        query.bind(1, username);
        query.bind(2, password);
        query.bind(3, salt);
        query.bind(4, email);
        query.exec();

    } catch (std::exception& e)
    {
        std::cerr << "Unexpected error when insert user: " << e.what() << std::endl;
        throw;
    }
}

// 函数原型改变：接收一个数据库引用
void test_query(SQLite::Database& db)
{
    std::cout << "test query" << std::endl;
    try
    {
        SQLite::Statement query(db,
            R"(
            SELECT * FROM users WHERE username = ?;
            )"
        );
        query.bind(1, "away");
        if (query.executeStep())
        {
            std::cout << "User found: " << query.getColumn(1).getString() << std::endl;
        }
        else
        {
            std::cout << "User not found" << std::endl;
        }
    } catch (std::exception& e)
    {
        std::cerr << "Unexpected error when query user: " << e.what() << std::endl;
        throw; // 重新抛出
    }
}


void test()
{
    try
    {
        SQLite::Database db(DB_PATH, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        std::cout << "Database opened." << std::endl;

        init_tables(db);
        test_insert(db);
        test_query(db);

    } catch (std::exception& e)
    {
        std::cerr << "\n--- An error occurred during test ---" << std::endl;
        std::cerr << e.what() << std::endl;
    }

    std::cout << "Test finished." << std::endl;
}

int main() {
    test();
    return 0;
}