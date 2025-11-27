//
// Created by AWAY on 25-10-14.
//

#pragma once

#include "httpserver.h"
#include "WebSocket.h"
#include <cassert>
#include "leptjson.h"
#include "types.h"

namespace uno
{
    class Session {
    public:
        enum class State
        {
            connected,
            authed,
            create_role,
            create_role_wait,
            logged,
            gaming,
        };

        using SessionPtr = std::shared_ptr<Session>;

        Session(WsSessionPtr ws) :
            m_last_save_time(std::chrono::system_clock::time_point::min())
        {
            m_wsSession = ws;
        }

        Session()
        {
            m_last_save_time = std::chrono::system_clock::now();
        }

        ~Session() = default;

        static SessionPtr create(WsSessionPtr ws)
        {
            return std::make_shared<Session>(ws);
        }

        static SessionPtr create()
        {
            return std::make_shared<Session>();
        }

        WsSessionPtr socket() const
        {
            return m_wsSession;
        }

        int uid() const
        {
            return m_uid;
        }

        const std::string& name() const
        {
            return m_name;
        }

        State state() const
        {
            return m_state;
        }

        const std::string& nick() const
        {
            return m_nick;
        }

        const std::string& email() const
        {
            return m_email;
        }

        const lept_value& summary() const
        {
            return m_summary;
        }

        const lept_value& data() const
        {
            return m_data;
        }

        lept_value& data()
        {
            return m_data;
        }

        bool dirty() const
        {
            return m_dirty;
        }

        std::chrono::system_clock::time_point last_save_time() const
        {
            return m_last_save_time;
        }

        void set_state(State s)
        {
            m_state = s;
        }

        void set_dirty()
        {
            m_dirty = true;
        }

        void set_undirty(const std::chrono::system_clock::time_point& now)
        {
            m_dirty = false;
            m_last_save_time = now;
        }

        void attach(int uid, const std::string& name, const std::string& email)
        {
            assert(m_uid == -1);
            m_uid = uid;
            m_name = name;
            m_email = email;
            //todo: emit('attach')
        }

        void attach_data(const std::string& nick,
                        const lept_value& summary,
                        const lept_value& data)
        {
            m_nick = nick;
            m_summary = summary;
            m_data = data;
        }

        //todo call(funcname, ...args)
        template <typename... Args>
        void call(const std::string& funcname, Args&&... args)
        {
            //todo emit('s2c', funcname, ...args)
            std::vector<lept_value> arr;
            arr.emplace_back(lept_value("s2c"));
            arr.emplace_back(lept_value(funcname));
            // 折叠表达式，解包
            (arr.emplace_back(lept_value(std::forward<Args>(args))), ...);
            lept_value lv(std::move(arr));

            m_wsSession->send(lv);
        }

    private:
        WsSessionPtr m_wsSession;

        int m_uid = -1;
        std::string m_name;
        std::string m_email;
        State m_state = State::connected;

        std::string m_nick;
        lept_value m_summary;
        lept_value m_data;

        bool m_dirty = false;
        std::chrono::system_clock::time_point m_last_save_time;

    };

    using SessionPtr = std::shared_ptr<Session>;
}

