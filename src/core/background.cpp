//
// Created by AWAY on 25-10-19.
//

#include "background.h"

namespace uno
{

    template<typename T>
    void Background::submit(
        std::function<T()> task,
        std::function<void(BackResult<T>)> callback)
    {
        std::function<void()> back_task = [this, task, callback]()
        {
            BackResult<T> result;
            try {
                result = task();
            } catch (std::exception& e) {
                result = std::current_exception();
            }

            std::function<void()> loop_cb = [callback, result]()
            {
                callback(result);
            };
        };

        m_taskQue.push(back_task);
        call_loop();
    }

    void Background::submit(
        std::function<void()> task,
        std::function<void(std::exception_ptr)> callback)
    {
        std::function<void()> back_task = [this, task, callback]()
        {
            std::exception_ptr err_ptr = nullptr;
            try {
                task();
            } catch (std::exception& e) {
                err_ptr = std::current_exception();
            }

            std::function<void()> loop_cb = [callback, err_ptr]()
            {
               callback(err_ptr);
            };

            m_taskQue.push(loop_cb);
            call_loop();
        };

    }

    bool Background::try_do_task()
    {
        auto task = m_taskQue.try_pop();
        if (task == std::nullopt)
        {
            return false;
        }

        task.value()();

        return true;
    }

    void Background::do_task()
    {
        auto task = m_taskQue.pop();

        if (task != std::nullopt && task)
        {
            task.value()();
        }
    }

    void Background::call_loop()
    {
        uv_async_send(m_async);
    }

    void Background::start()
    {
        if (m_running)
        {
            std::cerr << "already running" << std::endl;
            return ;
        }

        m_running = true;
        m_thread.emplace(&thread_loop, this);
        std::cout << "background thread started" << m_thread->get_id() << std::endl;
    }

    void Background::stop()
    {
        if (!m_running)
        {
            return ;
        }

        std::cout << "stopping background thread" << std::endl;

        m_running = false;
        m_taskQue.stop();

    }

    void Background::thread_loop()
    {
        while (m_running)
        {
            do_task();
        }

        std::cout << "background thread stopped" << std::endl;
    }


}