//
// Created by AWAY on 25-10-28.
//

#pragma once

#include <queue>
#include <thread>
#include <mutex>

namespace uno {

    template<typename T>
    class ThreadQue {
    public:
        ThreadQue() = default;
        ~ThreadQue() = default;

        /**
          * 禁止复制构造函数和赋值运算符
          */
        ThreadQue(const ThreadQue&) = delete;
        ThreadQue& operator=(const ThreadQue&) = delete;

        void push(T value);

        /**
          * @return 如果队列不为空，则返回弹出的值；否则返回 std::nullopt
          */
        std::optional<T> pop();

        size_t size();
        bool empty();



    private:
        std::queue<T> m_que;
        std::mutex m_mutex;

    };

};





