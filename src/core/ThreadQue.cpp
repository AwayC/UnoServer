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
    }

    template<typename T>
    std::optional<T> ThreadQue<T>::pop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_que.empty()) {
            return std::nullopt;
        }

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