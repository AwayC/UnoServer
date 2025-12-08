//
// Created by AWAY on 25-10-14.
//

#include "server.h"
#include "errc.h"
#include <regex>
#include <utility>
#include "db.h"
#include "../game/room.h"
#include "../game/lobby.h"

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

            std::cout << "data: " << data.stringify() << std::endl;
            int uid = data["uid"].get_integer();
            this->m_db.users_update_login_info(uid, ip);

            //返回 jwt
            std::string token = JwtUtil::sign({
                {"uid", uid},
                {"username", data["username"].get_string()},
                {"email", data["email"].get_string()}
            }, this->m_cfg["secret"].get_string(), 30);
            std::cout << "token: " << token << std::endl;
            return std::make_pair(ErrCode::ok, token);
        };

        m_bg.submit<std::pair<ErrCode, std::string>>(task, cb);
    }

    void Server::registerRouter()
    {
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

    void Server::onDbConnected()
    {
        std::cout << "Database connected" << std::endl;
        m_bg.start();

        // 开启 background thread
        std::cout << "Background thread started" << std::endl;

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


    void Server::onSocketConnection(const WsSessionPtr& socket)
    {
        std::cout << "Accept new socket" << std::endl;
        // todo ssmgr.accept(socket);
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

    void Server::pageIndex(httpReq* req, httpRespPtr resp)
    {
        resp->sendFile(m_staticDir + "index.html");
    }

    void Server::pageRegister(httpReq* req, httpRespPtr resp)
    {
        std::cout << "http Register" << std::endl;
        std::cout << "req body: " << req->body << std::endl;

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

        std::cout << "ip: " << ip << std::endl;
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
        std::cout << "name: " << argName << std::endl;
        std::cout << "password: " << argPassword << std::endl;
        std::cout << "email: " << argEmail << std::endl;

        std::cout << argPassword.size() << std::endl;

        if (argName.length() > 16)
        {
            std::cout << "name too long" << std::endl;
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_name_too_long)},
                {"msg", "Name too long"}
            });
            return;
        }

        if (argName.length() < 3)
        {
            std::cout << "name too short" << std::endl;
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_name_too_short)},
                {"msg", "Name too short"}
            });
            return;
        }

        if (argPassword.length() > 32)
        {
            std::cout << "password too long" << std::endl;
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_password_too_long)},
                {"msg", "Password too long"}
            });
            return;
        }

        if (argPassword.length() < 3)
        {
            std::cout << "password too short" << std::endl;
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_password_too_short)},
                {"msg", "Password too short"}
            });
            return;
        }

        if (argEmail.length() > 100)
        {
            std::cout << "email too long" << std::endl;
            resp->sendJson({
                {"code", static_cast<int>(ErrCode::api_email_invalid)},
                {"msg", "Invalid email"}
            });
            return;
        }

        if (!std::regex_match(argName, std::regex("^[A-Za-z0-9_]+$"))) {
            std::cout << "name invalid" << std::endl;
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
                std::cout << "hasName: " << hasName << std::endl;
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
        std::cout << "http Login" << std::endl;
        std::cout << "req body: " << req->body << std::endl;

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
                std::cout << "code: " << static_cast<int>(code) << std::endl;
                token = std::move(ref.second);
                std::cout << "token: " << token << std::endl;
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
        std::cout << "http UpdateEmail" << std::endl;
        std::cout << "req body: " << req->body << std::endl;

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
                std::cerr << "Unexcepted error when update email, uid: " << uid << std::endl;
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
        std::cout << "http UpdatePassword" << std::endl;
        std::cout << "req body: " << req->body << std::endl;

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
                std::cerr << "Unexcepted error when update password, uid: " << uid << std::endl;
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
        std::cout << "start background thread" << std::endl;

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