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
        };


    }

}