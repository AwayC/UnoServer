//
// Created by AWAY on 25-11-6.
//
#include <uv.h>
#include "core/db.h"
#include "sqlite3.h"
#include "sql/mysql.h"

using namespace uno;

#define DB_PATH "mysqlx://root:123456@localhost:33060/test"
#define EXCEPT_CB_TRY(err, msg) do { \
        try \
        { \
            if ((err)) \
                std::rethrow_exception((err)); \
            std::cout << (msg) << std::endl; \
        } catch (const std::exception& e) \
        {  \
            std::cout << (msg) << " err: " << e.what() << std::endl; \
            uv_timer_stop(&g_timer1); \
        } \
    } while (0)

ThreadQue<BackCallback> g_que;
uv_loop_t* g_loop;
Background* g_bg;
MysqlAgent* g_db;
uv_async_t g_async;
uv_timer_t g_timer1;
uv_timer_t g_timer2;

void background_handler(uv_async_t* handle)
{
    while (!g_que.empty())
    {
        auto task = g_que.try_pop();
        if (task != std::nullopt && task.value())
        {
            task.value()();
        }
    }
}

void test_db_funcs(uv_timer_t* handle)
{
    static int count_times = 0, has_user = 0, has_nickname = 0;
    count_times ++;

    if (count_times > 16)
    {
        std::cout << std::endl << "test_db done" << std::endl;
        uv_timer_stop(&g_timer1);
        return;
    }

    std::cout << "test_db_funcs " << count_times << std::endl;

    /**
     *  1. conntect db
     *  2. users has username
     *  3. users has uid
     *  4. users find uid by name
     *  5. users create user
     *  6. users validate password
     *  7. usres query user
     *  8. users update login info
     *  9. users update email
     *  10. users update password
     *  11. uno game players has role
     *  12. uno game players has nickname
     *  13. uno game players create player
     *  14. uno game players load role
     *  15. uno game players save role
     *  16. uno game players load summary
     *
     */

    switch (count_times)
    {
    case 1:
        {
            g_db->connect([](std::exception_ptr err)
            {
                EXCEPT_CB_TRY(err, "connect db");
            });
            break;
        }
    case 2:
        {
            g_db->users_has_username("away", [](BackResult<bool> res)
            {
               try
               {
                   has_user = std::get<bool>(res);
                   std::cout << "has_username: " << has_user << std::endl;
               } catch (const std::exception& e)
               {
                   std::cout << e.what() << std::endl;
               }
            });

            break;
        }

    case 3:
        {
            if (has_user)
            {
                std::cout << "user already exists" << std::endl;
                break;
            }

            g_db->users_create_user("1234",
                            "123456",
                            "away@example.com",
            [](std::exception_ptr err)
            {
                EXCEPT_CB_TRY(err, "create user");
            });

            break;
        }
    case 4:
        {
            g_db->users_has_uid(1, [](BackResult<bool> res)
            {
               try
               {
                   int has_uid = std::get<bool>(res);
                   std::cout << "has_uid: " << has_uid << std::endl;
               } catch (const std::exception& e)
               {
                   std::cout << e.what() << std::endl;
               }
            });

            break;
        }
    case 5:
        {
            g_db->users_find_uid_by_name("away", [](BackResult<int> res)
            {
               try
               {
                   int uid = std::get<int>(res);
                   std::cout << "find uid: " << uid << std::endl;
               } catch (const std::exception& e)
               {
                   std::cout << e.what() << std::endl;
               }
            });

            break;
        }
    case 6:
        {
            g_db->users_validate_password("away", "123456", [](BackResult<bool> res)
            {
               try
               {
                   int valid = std::get<bool>(res);
                   std::cout << "validate password: " << valid << std::endl;
               } catch (const std::exception& e)
               {
                   std::cout << e.what() << std::endl;
               }
            });

            break;
        }
    case 7:
        {
            g_db->users_query_user("away", [](BackResult<lept_value> res)
            {
               try
               {
                   auto& user_info = std::get<lept_value>(res);
                   std::cout << "query user: " << user_info.stringify() << std::endl;
               } catch (const std::exception& e)
               {
                   std::cout << e.what() << std::endl;
               }
            });

            break;
        }
    case 8:
        {
            g_db->users_update_login_info(1, "192.168.1.1", [](std::exception_ptr err)
            {
               EXCEPT_CB_TRY(err, "update login_info");
            });

            break;
        }
    case 9:
        {
            g_db->users_update_email(1, "away2@example.com", [](std::exception_ptr err)
            {
                EXCEPT_CB_TRY(err, "update email");
            });

            break;
        }
    case 10:
        {
            g_db->users_update_password(1, "123456", [](std::exception_ptr err)
            {
                EXCEPT_CB_TRY(err, "update password");
            });

            break;
        }
    case 11:
        {
            g_db->uno_game_players_has_nickname("away", [](BackResult<bool> res)
            {
               try
               {
                   has_nickname = std::get<bool>(res);
                   std::cout << "uno game has_nickname: " << has_nickname << std::endl;
               } catch (const std::exception& e)
               {
                   std::cout << e.what() << std::endl;
               }
            });
            break;
        }
    case 12:
        {
            if (has_nickname)
            {
                std::cout << "nickname already exists" << std::endl;
                break;
            }

            g_db->uno_game_players_create_role(1, "away", [](std::exception_ptr err)
            {
                EXCEPT_CB_TRY(err, "uno game create role");
            });
            break;
        }
    case 13:
        {
            g_db->uno_game_players_has_role(1, [](BackResult<bool> res)
            {
               try
               {
                   bool has_role = std::get<bool>(res);
                   std::cout << "uno game has_role: " << has_role << std::endl;
               } catch (const std::exception& e)
               {
                   std::cout << e.what() << std::endl;
               }
            });
            break;
        }
    case 14:
        {
            g_db->uno_game_players_load_role(1, [](BackResult<lept_value> res)
            {
               try
               {
                   lept_value& role = std::get<lept_value>(res);
                   std::cout << "uno game load_role: " << role.stringify() << std::endl;
               } catch (const std::exception& e)
               {
                   std::cout << e.what() << std::endl;
               }
            });
            break;
        }
    case 15:
        {
            g_db->uno_game_players_save_role(1, lept_value({}),
                lept_value({{"nickname", "away"}}),
                [](std::exception_ptr err)
                {
                    EXCEPT_CB_TRY(err, "uno save role");
                });
            break;
        }
    case 16:
        {
            g_db->uno_game_players_load_summary(1, [](BackResult<lept_value> res)
            {
               try
               {
                   auto& summary = std::get<lept_value>(res);
                   std::cout << "uno game load_summary: " << summary.stringify() << std::endl;
               } catch (const std::exception& e)
               {
                   std::cout << e.what() << std::endl;
               }
            });
            break;
        }

    default:
        {
            uv_timer_stop(&g_timer1);
            break;
        }
    }
}

int main()
{
    g_loop = uv_default_loop();
    uv_async_init(g_loop, &g_async, background_handler);

    uv_timer_init(g_loop, &g_timer1);
    uv_timer_start(&g_timer1, test_db_funcs, 500, 500);

    g_bg = new Background(&g_que, &g_async);
    g_bg->start();

    g_db = new MysqlAgent(g_bg, DB_PATH);

    return uv_run(g_loop, UV_RUN_DEFAULT);
}