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
                      uv_loop_t* loop):
                m_cbQue(callbackQue),
                m_mainLoop(loop) { }

            ~Background();

            /**
             * 返回结果的任务
             */
            template<typename T>
            void submit(std::function<T()> task,
                        std::function<void(BackResult<T>)> callback);
            /**
             * 无返回值的任务
             */
            void submit(std::function<void()> task,
                        std::function<void(std::exception_ptr)> callback);


        private:
            ThreadQue<BackCallback>* m_cbQue;
            uv_loop_t* m_mainLoop;
            ThreadQue<BackTask> m_taskQue;


    };

}

