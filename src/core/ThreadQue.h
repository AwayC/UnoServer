//
// Created by AWAY on 25-10-28.
//

#pragma once

#include <queue>
#include <thread>
#include <mutex>
#include <optional>
#include <condition_variable>
#include <exception>

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

        void push(T value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_que.push(value);
            m_cond.notify_one();
        }

        /**
          * @return 如果队列不为空，则返回弹出的值；否则返回 std::nullopt
          */
        std::optional<T> try_pop()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_que.empty()) {
                return std::nullopt;
            }

            T value = m_que.front();
            m_que.pop();
            return value;
        }

        /**
         *
         * @return 阻塞, 停止时返回 std::nullopt
         */
        std::optional<T> pop(){
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond.wait(lock, [this]
            {
                return !m_que.empty() || m_done;
            });

            if (m_done)
            {
                return std::nullopt;
            }

            T value = m_que.front();
            m_que.pop();
            return value;
        }

        size_t size()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_que.size();
        }

        bool empty()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_que.empty();
        }


        void stop()
        {
            m_done = true;
            m_cond.notify_one();
        }

    private:
        std::queue<T> m_que;
        std::mutex m_mutex;
        std::condition_variable m_cond;
        bool m_done = false;

    };

};





