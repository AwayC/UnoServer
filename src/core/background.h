//
// Created by AWAY on 25-10-19.
//

#pragma once

#include <functional>
#include <thread>
#include <uv.h>
#include <iostream>
#include "ThreadQue.h"


/*

     user(main loop)
        [ callback, result, param ]

     ->

     db
      result , callback

     ->

     background thread
         result, callback

     ->

     main loop
          result, callback

 */

namespace uno {

    using BackTask = std::function<void()>;
    using BackCallback = std::function<void()>;

    template<typename T>
    using BackResult = std::variant<T, std::exception_ptr>;

    class Background {
        public:
            Background(ThreadQue<BackCallback>* callbackQue,
                      uv_async_t* async):
                m_cbQue(callbackQue),
                m_async(async) { }

            ~Background()
            {
                stop();
            };

            /**
             * 返回结果的任务, 要显式标注模版类型
             */
            template<typename T>
            void submit(std::function<T()> task,
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

                    m_cbQue->push(loop_cb);
                    call_loop();
                };

                m_taskQue.push(back_task);
            }
            /**
             * 无返回值的任务
             */
            void submit(std::function<void()> task,
                        std::function<void(std::exception_ptr)> callback);


            void start();

            void stop();

        private:
            ThreadQue<BackCallback>* m_cbQue;
            uv_async_t* m_async;
            ThreadQue<BackTask> m_taskQue;
            std::optional<std::thread> m_thread;
            std::atomic<bool> m_running;


            void call_loop();

            /**
             * 阻塞
             */
            void do_task();

            /**
             * 非阻塞
             */
            bool try_do_task();

            void thread_loop();

    };

}

