//
// Created by AWAY on 25-10-14.
//

#include "server.h"
#include "errc.h"
#include <regex>
#include "db.h"

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
                            std::function<void(ErrCode, std::string)> cb)
    {
        m_db.users_has_username(name, [=](BackResult<bool> res)
        {
            try
            {
                bool exit = std::get<bool>(res);
                if (!exit)
                {
                    cb(ErrCode::db_not_exists, "");
                    return;
                }
            } catch (const std::exception& e)
            {
                cb(ErrCode::api_db_error, "");
                return ;
            }

            this->m_db.users_validate_password(name, password, [=](BackResult<bool> res)
            {
                try
                {
                    bool validate = std::get<bool>(res);
                    if (!validate)
                    {
                        cb(ErrCode::api_bad_password, "");
                        return;
                    }
                } catch (const std::exception& e)
                {
                    cb(ErrCode::api_db_error, "");
                    return;
                }

                this->m_db.users_query_user(name, [=](BackResult<lept_value> res)
                {
                    try
                    {
                        lept_value& data = std::get<lept_value>(res);
                        int uid = data["uid"].get_number();
                        std::string ip = data["ip"].get_string();
                        this->m_db.users_update_login_info(uid, ip, [=](std::exception_ptr err)
                        {
                            if (err)
                            {
                                cb(ErrCode::api_db_error, "");
                            }
                            else
                            {
                                //todo jwt token
                                std::string token = "";
                                cb(ErrCode::ok, token);
                            }
                        });
                    } catch (const std::exception& e)
                    {
                        cb(ErrCode::api_db_error, "");
                        return;
                    }

                });
            });


        });
    }

    void Server::registerRouter()
    {
        m_httpSvr->post('/api/register', [this](httpReq* req,
                                            httpRespPtr resp)
        {
            pageRegister(req, resp);
        });
        m_httpSvr->post('/api/login', [this](httpReq* req, httpRespPtr resp)
        {
            pageLogin(req, std::move(resp));
        });
        m_httpSvr->post('/api/update_email', [this](httpReq* req, httpRespPtr resp)
        {
            pageUpdateEmail(req, std::move(resp));
        });
        m_httpSvr->post('/api/update_password', [this](httpReq* req, httpRespPtr resp)
        {
            pageUpdatePassword(req, std::move(resp));
        });

        this->m_wsSvr.onConnect([this](WsSessionPtr ss)
        {
            onSocketConnection(std::move(ss));
        });
    }

    void Server::onDbConnected()
    {
        std::cout << "Database connected" << std::endl;

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

    void onUpdate()
    {
        auto now = std::chrono::system_clock::now();

        try
        {
            //todo room.update(now);
        } catch (const std::exception& e)
        {
            std::cerr << "Error while update room state" << std::endl;
            std::cerr << e.what() << std::endl;
        }

        try
        {
            //todo ssmgr.update(now);
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

        m_db.users_has_username(argName, [=](BackResult<bool> res)
        {
            try
            {
               bool hasName = std::get<bool>(res);
               if (hasName)
               {
                   resp->sendJson({
                       {"code", static_cast<int>(ErrCode::db_exists)},
                       {"msg", "User existed"}
                   });
               }
            } catch (const std::exception& e)
            {
               RETHROW_EXCEPTION_PTR(res, "Unexpected error when create user, argName: " + argName);

               resp->sendJson({
                   {"code", static_cast<int>(ErrCode::api_internal_error)},
                   {"msg", "Db error"}
               });
               return ;
            }

            //todo internalLogin
            internalLogin(argName, argPassword, ip, [=](ErrCode code, std::string token)
            {
                if (code == ErrCode::ok)
                {
                    resp->sendJson({
                        {"code", static_cast<int>(code)},
                        {"msg", "Internal error"}
                    });
                }

                else if (code == ErrCode::api_db_error)
                {
                    std::cerr << "Unexpected error when login user, argName: " << argName << std::endl;
                    resp->sendJson({
                        {"code", static_cast<int>(ErrCode::api_internal_error)},
                        {"msg", "Db error"}
                    });
                }

                else
                {
                    std::cerr << "_internalLogin unexpected result: " << static_cast<int>(code) << std::endl;
                    resp->sendJson({
                        {"code", static_cast<int>(ErrCode::api_internal_error)},
                        {"msg", "Internal error"}
                    });
                }`

            });
        });
    }



}