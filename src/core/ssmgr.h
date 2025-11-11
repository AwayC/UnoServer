//
// Created by AWAY on 25-10-14.
//

#pragma once

#include "session.h"
#include "router.h"
#include <unordered_map>

#define SESSION_ITER_COUNT    10
#define SESSION_SAVE_DURATION (5 * 60 * 1000)

namespace uno
{
    using SessionPtr = std::shared_ptr<Session>;
    class Ssmgr {



    private:
        std::vector<Session*> m_sessions;
        std::unordered_map<int, SessionPtr> m_uid2session;
        size_t m_iter = 0;

        void on_error(const SessionPtr& session, const std::exception& err)
        {
            std::cerr << err.what() << std::endl;
        }

        template<typename... Args>
        void on_c2s_msg(const SessionPtr& session,
                        const std::string& funcname,
                        Args&&... args)
        {

        }
    };

}

