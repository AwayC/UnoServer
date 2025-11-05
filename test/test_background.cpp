//
// Created by AWAY on 25-11-4.
//

#include "core/background.h"
#include "uv.h"
#include <chrono>

#include "leptjson.h"

using namespace uno;

uv_loop_t *g_loop;
Background* g_bg;
uv_async_t g_async;
ThreadQue<BackCallback> g_que;
uv_timer_t g_timer1;
uv_timer_t g_timer2;

void background_handler(uv_async_t* handle)
{
    while (!g_que.empty())
    {
        auto task = g_que.try_pop();
        if (task != std::nullopt)
        {
            task.value()();
        }
    }
}

void test_no_result_submit(uv_timer_t* handle)
{
    static int count = 0;
    count ++;
    if (count > 2)
    {
        uv_timer_stop(&g_timer1);
    }

    g_bg->submit([=]()
    {
        std::cout << "no result submit " << count << std::endl;
    },
    [](std::exception_ptr err)
    {
        try
        {
            if (err)
            {
                std::rethrow_exception(err);
            } else
            {
                std::cout << "test_no_result_submit success" << std::endl;
            }
        } catch (std::exception& e)
        {
            std::cout << "err callback: " << e.what() << std::endl;
        }
    }
    );

}

void test_result_submit(uv_timer_t* handle)
{
    static int count = 0;
    count ++;

    /**
     * 1: bool
     * 2: int
     * 3: std::string
     * 4: lept_value
     */
    switch (count)
    {
        case 1:
        {
            g_bg->submit<bool>([=]()
            {
                return true;
            },
            [](BackResult<bool> result)
            {
                try
                {
                    std::get<bool>(result);
                    std::cout << "test_result_submit bool success, result: " << std::get<bool>(result) << std::endl;
                } catch (std::exception& e)
                {
                    std::get<std::exception_ptr>(result);
                    std::rethrow_exception(std::get<std::exception_ptr>(result));
                }
            });
            break;
        }
        case 2:
        {
            g_bg->submit<int>([=]()
            {
                return 100;
            },
            [](BackResult<int> result)
            {
                try
                {
                    std::get<int>(result);
                    std::cout << "test_result_submit int success, result: " << std::get<int>(result) << std::endl;
                } catch (std::exception& e)
                {
                    std::get<std::exception_ptr>(result);
                    std::rethrow_exception(std::get<std::exception_ptr>(result));
                }
            });
            break;
        }
        case 3:
        {
            g_bg->submit<std::string>([=]()->std::string
            {
                return "result";
            },
            [](BackResult<std::string> result)
            {
                try
                {
                    std::get<std::string>(result);
                    std::cout << "test_result_submit std::string success, result: " << std::get<std::string>(result) << std::endl;
                } catch (std::exception& e)
                {
                    std::get<std::exception_ptr>(result);
                    std::rethrow_exception(std::get<std::exception_ptr>(result));
                }
            });
            break;
        }
        case 4:
        {
            g_bg->submit<lept_value>([=]()->lept_value
            {
                return lept_value({
                    {"authoer", "away"},
                    {"age", 18},
                });
            },
            [](BackResult<lept_value> result)
            {
                try
                {
                    std::get<lept_value>(result);
                    std::cout << "test_result_submit lept_value success, result: "
                                << std::get<lept_value>(result).stringify() << std::endl;
                } catch (std::exception& e)
                {
                    std::get<std::exception_ptr>(result);
                    std::rethrow_exception(std::get<std::exception_ptr>(result));
                }
            });
            break;
        }

        default:
        {
            uv_timer_stop(&g_timer2);
            break;
        }

    }
}

int main()
{
    g_loop = uv_default_loop();
    uv_async_init(g_loop, &g_async, background_handler);

    uv_timer_init(g_loop, &g_timer1);
    uv_timer_start(&g_timer1, test_no_result_submit, 500, 1000);
    uv_timer_init(g_loop, &g_timer2);
    uv_timer_start(&g_timer2, test_result_submit, 4000, 1000);


    g_bg = new Background(&g_que, &g_async);
    g_bg->start();

    return uv_run(g_loop, UV_RUN_DEFAULT);
}
