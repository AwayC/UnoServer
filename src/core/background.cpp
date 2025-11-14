//
// Created by AWAY on 25-10-19.
//

#include "background.h"

namespace uno
{

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

            m_cbQue->push(loop_cb);
            call_loop();
        };

        m_taskQue.push(back_task);
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
        if (!m_running.exchange(true))
        {
            try
            {
                m_thread.emplace(&Background::thread_loop, this);
                if (m_thread)
                {
                    std::cout << "background thread started , id: " << m_thread->get_id() << std::endl;
                } else
                {
                    std::cerr << "background thread failed to start" << std::endl;
                }

            } catch (std::exception& e)
            {
                std::cerr << "background thread error: " << e.what() << std::endl;
                m_running = false;
            }

        }
        else
        {
            std::cerr << "background thread already running" << std::endl;
        }

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