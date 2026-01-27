//
// Created by AWAY on 25-10-14.
//

#include "server.h"
#include "errc.h"
#include <regex>
#include <utility>
#include <filesystem>

#include "db.h"
#include "../game/room.h"
#include "../game/lobby.h"
#include "../game/dbagent.h"

#define RETHROW_EXCEPTION_PTR(res, msg)                       \
    try {                                                    \
        std::exception_ptr err = std::get<std::exception_ptr>(res); \
        std::rethrow_exception(err);                         \
    } catch (const std::exception& e_) {                      \
        std::cerr << (msg) << std::endl;                       \
        std::cerr << e_.what() << std::endl;                  \
    }


namespace uno
{
    Server::Server(lept_value& cfg) :
                m_cfg(cfg),
                m_staticDir("./static"),
                m_httpSvr(HttpServer::create( cfg.contains_key("ip") ? cfg["ip"].get<std::string>() : UNO_SERVER_IP,
                    cfg.contains_key("port") ? cfg["port"].get<int>() : UNO_SERVER_PORT)),
                m_wsSvr(m_httpSvr),
                m_bg(&m_loop_ctx.que, &m_loop_ctx.async),
                m_db(&m_bg, cfg.contains_key("db_url") ? cfg["db_url"].get<std::string>() : UNO_DB_PATH),
                m_dbagent(&m_db)
    {
        std::cout << cfg.stringify() << std::endl;
        std::cout << m_cfg.stringify() << std::endl;
        std::cout << "server ip: " << UNO_SERVER_IP << std::endl;
        std::cout << "server port: " << (cfg.contains_key("port") ? cfg["port"].get<int>() : UNO_SERVER_PORT) << std::endl;

        //todo init server
        m_loop_ctx.loop = m_httpSvr->getLoop();
        m_loop_ctx.async.data = this;
        uv_async_init(m_loop_ctx.loop, &m_loop_ctx.async, background_handler);

        // init lobby register functions
        Lobby::instance();
        // init room manager register functions
        RoomManager::instance();
        registerRouter();
    }

    void Server::internalLogin(const std::string& name,
                            const std::string& password,
                            const std::string& ip,
                            const std::function<void(BackResult<std::pair<ErrCode, std::string>>)>& cb)
    {
        std::cout << "internalLogin" << std::endl;
        auto task = [=, this]() -> std::pair<ErrCode, std::string>
        {
            bool exit = this->m_db.users_has_username(name);
            if (!exit)
            {
                return std::make_pair(ErrCode::db_not_exists, std::string("user not exists"));
            }

            bool validate = this->m_db.users_validate_password(name, password);
            if (!validate)
            {
                return std::make_pair(ErrCode::db_not_exists, std::string("password not match"));
            }

            lept_value data = this->m_db.users_query_user(name);

            int uid = data["uid"].get_integer();
            this->m_db.users_update_login_info(uid, ip);

            //返回 jwt
            std::string token = JwtUtil::sign({
                {"uid", uid},
                {"username", data["username"].get_string()},
                {"email", data["email"].get_string()}
            }, this->m_cfg["secret"].get_string(), 30);
            return std::make_pair(ErrCode::ok, token);
        };

        m_bg.submit<std::pair<ErrCode, std::string>>(task, cb);
    }

    void Server::registerRouter()
    {
        m_httpSvr->get("/(.*)", [this](httpReq* req, httpRespPtr resp)
        {
            pageStatic(req, std::move(resp));
        });

        m_httpSvr->post("/api/register", [this](httpReq* req,
                                            httpRespPtr resp)
        {
            pageRegister(req, std::move(resp));
        });
        m_httpSvr->post("/api/login", [this](httpReq* req, httpRespPtr resp)
        {
            pageLogin(req, std::move(resp));
        });
        m_httpSvr->post("/api/update_email", [this](httpReq* req, httpRespPtr resp)
        {
            pageUpdateEmail(req, std::move(resp));
        });
        m_httpSvr->post("/api/update_password", [this](httpReq* req, httpRespPtr resp)
        {
            pageUpdatePassword(req, std::move(resp));
        });


        this->m_wsSvr.onConnect([this](WsSessionPtr ss)
        {
            onSocketConnection(ss);
        });
    }

    void Server::startUpdateTimer()
    {
        uv_timer_init(m_loop_ctx.loop, &m_updateTimer);
        m_updateTimer.data = this;
        uv_timer_start(&m_updateTimer, [](uv_timer_t* handle)
        {
            auto self = static_cast<Server*>(handle->data);
            self->onUpdate();
        }, 100, 100);
    }

    void Server::stopUpdateTimer()
    {
        uv_timer_stop(&m_updateTimer);
    }

    void Server::onDbConnected()
    {
        std::cout << "Database connected" << std::endl;
        m_bg.start();

        // 开启 background thread
        std::cout << "Background thread started" << std::endl;

        // 主Tick循环
        startUpdateTimer();

        // 开启监听
        m_httpSvr->start();
    }

    void Server::onDbConnectFailed()
    {
        std::cout << "Database connect failed" << std::endl;
        exit(1);
    }

    void Server::onServerListen()
    {
        std::cout << "Server listening" << std::endl;
    }


    void Server::onSocketConnection(WsSessionPtr& socket)
    {
        std::cout << "Accept new socket" << std::endl;
        Ssmgr::instance().accept(socket);
    }

    void Server::onUpdate()
    {
        auto now = std::chrono::system_clock::now();

        try
        {
            //todo room.update(now);
            RoomManager::instance().update(now);
        } catch (const std::exception& e)
        {
            std::cerr << "Error while update room state" << std::endl;
            std::cerr << e.what() << std::endl;
        }

        try
        {
            //todo ssmgr.update(now);
            Ssmgr::instance().update(now);
        } catch (const std::exception& e)
        {
            std::cerr << "Error while update session state" << std::endl;
            std::cerr << e.what() << std::endl;
        }
    }

    void Server::pageStatic(httpReq* req, httpRespPtr resp)
    {
        std::string& path = req->url;
        if (path == "/")
        {
            path = "/index.html";
        }

        std::cout << "request path: " << path << std::endl;
        // 移除查询参数
        size_t query_pos = path.find('?');
        if (query_pos != std::string::npos) {
            path = path.substr(0, query_pos);
        }

        if (path.find("..") != std::string::npos) {
            std::cerr << "Forbidden path: " << path << std::endl;
            resp->setStatus(httpStatus::FORBIDDEN);
            resp->sendStr("403 Forbidden");
            return;
        }

        std::string static_path = m_staticDir + path;

        if (!std::filesystem::exists(static_path) || std::filesystem::is_directory(static_path)) {
            std::cout << "not found: " << static_path << std::endl;
            resp->setStatus(httpStatus::NOT_FOUND);
            resp->sendStr("404 Not Found");
            return;
        }
        std::cout << static_path << std::endl;

        resp->sendFile(static_path);
        std::cout << "send file: " << static_path << std::endl;
    }


    void Server::pageRegister(httpReq* req, httpRespPtr resp)
    {
        std::string ip;
        std::string argName, argPassword, argEmail;
        if (req->headers.find("x-forwarded-for") != req->headers.end())
        {
            ip = req->headers["x-forwarded-for"];
        }
        else
        {
            ip = req->ip;
        }

        try
        {
            lept_value body;
            body.parse(req->body);

            argName = body["name"].get_string();
            argPassword = body["password"].get_string();
            argEmail = body["email"].get_string();
        } catch (const std::exception& e)
        {
            std::cerr << "Error while registering page" << std::endl;
            std::cerr << e.what() << std::endl;
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_bad_req)},
                {"msg", "Bad request"}
            });
            return ;
        }

        if (argName.length() > 16)
        {
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_name_too_long)},
                {"msg", "Name too long"}
            });
            return;
        }

        if (argName.length() < 3)
        {
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_name_too_short)},
                {"msg", "Name too short"}
            });
            return;
        }

        if (argPassword.length() > 32)
        {
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_password_too_long)},
                {"msg", "Password too long"}
            });
            return;
        }

        if (argPassword.length() < 3)
        {
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_password_too_short)},
                {"msg", "Password too short"}
            });
            return;
        }

        if (argEmail.length() > 100)
        {
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_email_invalid)},
                {"msg", "Invalid email"}
            });
            return;
        }

        if (!std::regex_match(argName, std::regex("^[A-Za-z0-9_]+$"))) {
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_name_invalid)},
                {"msg", "Invalid name"}
            });
            return;
        }

        std::regex re(
            R"(^(([^<>()\[\]\\.,;:\s@"]+(\.[^<>()\[\]\\.,;:\s@"]+)*)|(".+"))@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\])|(([a-zA-Z\-0-9]+\.)+[a-zA-Z]{2,}))$)"
        );
        std::string emailLower = argEmail;
        std::transform(emailLower.begin(), emailLower.end(), emailLower.begin(), ::tolower);

        if (!std::regex_match(emailLower, re)) {
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_email_invalid)},
                {"msg", "Invalid email"}
            });
            return;
        }

        auto create_user_task = [=, this]()
        {
            bool hasName = m_db.users_has_username(argName);
            if (!hasName)
            {
                m_db.users_create_user(argName, argPassword, argEmail);
            }
            return hasName;
        };
        m_bg.submit<bool>(create_user_task, [=, this](BackResult<bool> res)
        {
            if (res.index() == 0)
            {
                bool hasName = std::get<0>(res);
                if (hasName)
                {
                    resp->sendJson({
                        {"code", static_cast<int>(ErrCode::db_exists)},
                        {"msg", "User existed"}
                    });
                    return ;
                }
            } else
            {
                std::cerr << "Unexpected error when create user, argName: " << argName << std::endl;
                resp->sendJson({
                    {"code", static_cast<int>(ErrCode::api_internal_error)},
                    {"msg", "Db error"}
                });
            }

            //todo internalLogin
            internalLogin(argName, argPassword, ip, [=, this](BackResult<std::pair<ErrCode, std::string>> res)
            {
                ErrCode code;
                std::string token;

                if (res.index() == 0)
                {
                    auto& ref = std::get<0>(res);
                    code = ref.first;
                    token = std::move(ref.second);
                } else
                {
                    std::cerr << "Unexpected error when login user, argName: " << argName << std::endl;
                    resp->sendJson({
                        {"code", static_cast<int>(ErrCode::api_internal_error)},
                        {"msg", "Db error"}
                    });
                    return ;
                }

                if (code != ErrCode::ok)
                {
                    std::cerr << "internalLogin unexpected result: " << token << std::endl;
                    resp->sendJson({
                        {"code", static_cast<int>(ErrCode::api_internal_error)},
                        {"msg", "Internal error"}
                    });
                    return ;
                }

                resp->sendJson({
                    {"code", static_cast<int>(ErrCode::ok)},
                    {"msg", "OK"},
                    {"data", {{"token", token}} }
                });

            });
        });
    }

    void Server::pageLogin(httpReq* req, httpRespPtr resp)
    {
        std::string ip;
        std::string argName, argPassword;

        if (req->headers.find("x-forwarded-for") != req->headers.end())
        {
            ip = req->headers["x-forwarded-for"];
        } else
        {
            ip = req->ip;
        }

        try
        {
            lept_value body;
            body.parse(req->body);
            argName = body["name"].get_string();
            argPassword = body["password"].get_string();
        } catch (const std::exception& e)
        {
            std::cerr << "pageLogin error: " << e.what() << std::endl;
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_bad_req)},
                {"msg", "Bad request"}
            });
            return ;
        }

        internalLogin(argName, argPassword, ip, [=](BackResult<std::pair<ErrCode, std::string>> res)
        {
            ErrCode code;
            std::string token;

            if (res.index() == 0)
            {
                auto ref = std::get<std::pair<ErrCode, std::string>>(res);
                code = ref.first;
                token = std::move(ref.second);
            } else
            {
                std::cerr << "Unexpected error when login user, argName: " << argName << std::endl;

                resp->sendJson({
                    {"code", static_cast<int>(ErrCode::api_internal_error)},
                    {"msg", "Db error"}
                });
                return ;
            }

            if (code != ErrCode::ok)
            {
                resp->sendJson({
                    {"code", static_cast<int>(ErrCode::api_bad_password)},
                    {"msg", "Bad password"}
                });
                return ;
            }

            resp->sendJson({
                {"code", static_cast<int>(ErrCode::ok)},
                {"msg", "OK"},
                {"data", {{"token", token}} }
            });

        });
    }

    void Server::pageUpdateEmail(httpReq* req, httpRespPtr resp)
    {
        std::string argToken, argEmail;

        try
        {
            lept_value body;
            body.parse(req->body);

            argToken = body["token"].get_string();
            argEmail = body["email"].get_string();
        } catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_bad_req)},
                {"msg", "Bad request"}
            });
        }

        if (argEmail.length() > 100)
        {
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_internal_error)},
                {"msg", "Email error"}
            });
        }

        std::regex re(
            R"(^(([^<>()\[\]\\.,;:\s@"]+(\.[^<>()\[\]\\.,;:\s@"]+)*)|(".+"))@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\])|(([a-zA-Z\-0-9]+\.)+[a-zA-Z]{2,}))$)"
        );
        std::string emailLower = argEmail;
        std::transform(emailLower.begin(), emailLower.end(), emailLower.begin(), ::tolower);

        if (!std::regex_match(emailLower, re)) {
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_email_invalid)},
                {"msg", "Invalid email"}
            });
            return;
        }

        lept_value payload;
        try
        {
           payload = JwtUtil::verify(argToken,
                                    this->m_cfg["secret"].get_string(),
                                    12);
        } catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_bad_token)}
            });
        }

        int uid = payload["uid"].get_integer();
        assert(uid);

        m_db.users_update_email(uid, argEmail, [=](std::exception_ptr err)
        {
            if (err)
            {
                std::cerr << "Unexpected error when update email, uid: " << uid << std::endl;
                resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_internal_error)},
                {"msg", "Db error"}
                });

                return ;
            }

            resp->sendJson({
                {"code", static_cast<int>(ErrCode::ok)},
                {"msg", "OK"}
            });
        });
    }

    void Server::pageUpdatePassword(httpReq* req, httpRespPtr resp)
    {
        std::string argToken, argPassword;
        try
        {
            lept_value body;
            body.parse(req->body);

            argToken = body["token"].get_string();
            argPassword = body["password"].get_string();
        } catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_bad_password)},
                {"msg", "Bad password"}
            });
        }

        if (argPassword.length() > 32)
        {
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_password_too_long)},
                {"msg", "Password too long"}
            });
            return ;
        }

        if (argPassword.length() < 3)
        {
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_password_too_short)},
                {"msg", "Password too short"}
            });
            return ;
        }

        lept_value payload;
        try
        {
            payload = JwtUtil::verify(argToken,
                                    this->m_cfg["secret"].get_string(),
                                    12);
        } catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_bad_password)},
                {"msg", "Bad token"}
            });
        }

        int uid = payload["uid"].get_integer();
        assert(uid);

        m_db.users_update_password(uid, argPassword, [=](std::exception_ptr err)
        {
            if (err)
            {
                std::cerr << "Unexpected error when update password, uid: " << uid << std::endl;
                resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_internal_error)},
                {"msg", "Db error"}
                });
                return ;
            }

            resp->sendJson({
                {"code", static_cast<int>(ErrCode::ok)},
                {"msg", "OK"}
            });
        });
    }

    void Server::run()
    {
        std::cout << "server start" << std::endl;

        try
        {
            m_db.connect();
        } catch (const std::exception& e)
        {
            this->onDbConnectFailed();
        }

        this->onDbConnected();
    }

}