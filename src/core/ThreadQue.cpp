    //
// Created by AWAY on 25-10-28.
//

#include "ThreadQue.h"


namespace uno {

    template<typename T>
    void ThreadQue<T>::push(T value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_que.push(value);
        m_cond.notify_one();
    }

    template<typename T>
    std::optional<T> ThreadQue<T>::try_pop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_que.empty()) {
            return std::nullopt;
        }

        T value = m_que.pop();
        return value;
    }

    template<typename T>
    T ThreadQue<T>::pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [this]
        {
            return m_que.empty();
        });

        T value = m_que.pop();
        return value;
    }

    template<typename T>
    bool ThreadQue<T>::empty()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_que.empty();
    }

    template<typename T>
    size_t ThreadQue<T>::size()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_que.size();
    }

};