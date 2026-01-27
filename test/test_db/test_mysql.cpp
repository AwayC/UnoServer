//
// Created by AWAY on 25-10-29.
//
#include <iostream>
#include <exception>
#include <memory>
#include <mysqlx/xdevapi.h>

#define DB_URL "mysqlx://root:123456@localhost:33060/test"

using namespace mysqlx;

void init_tables(Client& cli)
{
    std::cout << "test connect (init tables)" << std::endl;
    try {
        Session sess = cli.getSession();
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

void test_insert(Client& cli)
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
        Session sess = cli.getSession();

        SqlResult result = sess.sql("SELECT uid FROM users WHERE username = ?")
                               .bind(username)
                               .execute();

        Row row = result.fetchOne();
        if (row) {
            has_user = true;
        }

    } catch (const Error& e)
    {
        std::cerr << "Unexpected error when query user: " << e << std::endl;
        throw;
    }

    if (has_user)
    {
        std::cout << "user exists" << std::endl;
        return ;
    }

    try
    {
        Session sess = cli.getSession();

        sess.sql(R"(
            INSERT INTO users (username, password, salt, email, register_time)
            VALUES (?, ?, ?, ?, NOW());
        )")
        .bind(username, password, salt, email)
        .execute();

    } catch (const Error& e)
    {
        std::cerr << "Unexpected error when insert user: " << e << std::endl;
        throw;
    }
}

void test_query(Client& cli)
{
    std::cout << "test query" << std::endl;
    try
    {
        Session sess = cli.getSession();

        SqlResult result = sess.sql("SELECT * FROM users WHERE username = ?")
                               .bind("away")
                               .execute();
        
        Row row = result.fetchOne();
        if (row)
        {
            std::cout << "User found: " << row[1] << std::endl;
        }
        else
        {
            std::cout << "User not found" << std::endl;
        }
    } catch (const Error& e)
    {
        std::cerr << "Unexpected error when query user: " << e << std::endl;
        throw; 
    }
}


void test()
{
    try
    {
        Client cli(DB_URL, ClientOption::POOL_MAX_SIZE, 5);
        std::cout << "Client created (pool init)." << std::endl;

        init_tables(cli);
        test_insert(cli);
        test_query(cli);

    } catch (const Error& e)
    {
        std::cerr << "\n--- An error occurred during test ---" << std::endl;
        std::cerr << e << std::endl;
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
