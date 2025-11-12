//
// Created by AWAY on 25-10-14.
//

#pragma once

#include "httpserver.h"
#include "WebSocket.h"
#include <assert.h>
#include "leptjson.h"
#include "router.h"
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

        Session(WsSessionPtr ws)
        {
            m_wsSession = ws;
        }

        WsSessionPtr socket() const
        {
            return m_wsSession;
        }

        int uid() const
        {
            return m_uid;
        }

        std::string name() const
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

        const summary_t& summary() const
        {
            return m_summary;
        }

        const data_t& data() const
        {
            return m_data;
        }

        bool dirty() const
        {
            return m_dirty;
        }

        int last_save_time() const
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

        void set_undirty()
        {
            m_dirty = false;
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
                        const summary_t& summary,
                        const data_t& data)
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
            arr.emplace_back("s2c");
            arr.emplace_back(funcname);
            // 折叠表达式，解包
            (arr.emplace_back(std::forward<Args>(args)), ...);
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
        summary_t m_summary;
        data_t m_data;

        bool m_dirty = false;
        int m_last_save_time = 0;



    };
}

