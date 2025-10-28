//
// Created by AWAY on 25-9-29.
//

#include "httpResp.h"

#include <cassert>
#include "httpserver.h"

std::string httpStatus_str(httpStatus status)
{

#define STATUS_STR(status, str) case httpStatus::status: return str
    switch (status)
    {
        STATUS_STR(SWITCHING_PROTOCOLS, "101 Web Socket Protocol Handshake");
        STATUS_STR(OK, "200 OK");
        STATUS_STR(CREATED, "201 Created");
        STATUS_STR(ACCEPTED, "202 Accepted");
        STATUS_STR(NO_CONTENT, "204 No Content");
        STATUS_STR(MOVED_PERMANENTLY, "301 Moved Permanently");
        STATUS_STR(FOUND, "302 Found");
        STATUS_STR(NOT_MODIFIED, "304 Not Modified");
        STATUS_STR(BAD_REQUEST, "400 Bad Request");
        STATUS_STR(UNAUTHORIZED, "401 Unauthorized");
        STATUS_STR(FORBIDDEN, "403 Forbidden");
        STATUS_STR(NOT_FOUND, "404 Not Found");
        STATUS_STR(INTERNAL_SERVER_ERROR, "500 Internal Server Error");
        STATUS_STR(NOT_IMPLEMENTED, "501 Not Implemented");
        STATUS_STR(BAD_GATEWAY, "502 Bad Gateway");
        STATUS_STR(SERVICE_UNAVAILABLE, "503 Service Unavailable");
        STATUS_STR(GATEWAY_TIMEOUT, "504 Gateway Timeout");
    default:
        return "Unknown Status";
    }

#undef STATUS_STR
}

/********** httpResp *********/
template httpResp::httpResp(std::shared_ptr<HttpServer::Session> ctx);

template<typename T>
httpResp::httpResp(std::shared_ptr<T> ctx)
{
    m_status = httpStatus::OK;
    m_sendType = SendType::NONE;
    m_client = ctx->getClient();

}

void httpResp::initReader(FileReader* reader)
{
    auto self = shared_from_this();
    reader->onError([self](FileReader* reader)
    {
        std::cerr << "FileReader: Error" << std::endl;
        self->sendErr();
        delete reader;
    });
    reader->onClose([self](FileReader* reader)
    {
        self->send();
    });
}

httpResp::~httpResp()
{
    std::cerr << "httpResp::~httpResp()" << std::endl;
}

void httpResp::sendFile(const std::string& path)
{
    m_headers["Content-Type"] = "application/octet-stream";
    m_filePath = path;
    m_sendType = SendType::FILE;
    // read file
    m_reader = new FileReader(m_client->loop);
    initReader(m_reader);
    m_reader->fileRead(m_filePath);
}

void httpResp::sendJson(const lept_value& json)
{
    m_body = json.stringify();
    m_headers["Content-Type"] = "application/json";
    m_sendType = SendType::JSON;

    send();
}

void httpResp::sendStr(const std::string& str)
{
    m_body = str;
    m_headers["Content-Type"] = "text/plain";
    m_sendType = SendType::STR;

    send();
}

void httpResp::sendStr(std::string&& str)
{
    m_body = std::move(str);
    m_headers["Content-Type"] = "text/plain";
    m_sendType = SendType::STR;
    send();
}

void httpResp::setHeader(const std::string& key, const std::string& value)
{
    m_headers[key] = value;
}

void httpResp::setStatus(httpStatus status)
{
    m_status = status;
}

void httpResp::send()
{
    m_head = "HTTP/1.1 " + httpStatus_str(m_status) + "\r\n";
    for (const auto& header : m_headers)
    {
        m_head.append(header.first + ": " + header.second + "\r\n");
    }

    if (m_sendType == SendType::FILE)
    {
        m_head.append("Content-Length: " + std::to_string(m_reader->getReadByte()) + "\r\n");
    } else
    {
        m_head.append("Content-Length: " + std::to_string(m_body.size()) + "\r\n");
    }

    m_head.append("\r\n");

    m_buffers.clear();
    m_buffers.push_back(uv_buf_init(const_cast<char*>(m_head.c_str()), m_head.size()));

    if (m_sendType == SendType::FILE)
    {
        m_reader->appendToBuff(m_buffers);
    } else
    {
        m_buffers.push_back(uv_buf_init(const_cast<char*>(m_body.c_str()), m_body.size()));
    }

    auto ctx = new WriteContext();
    ctx->req.data = ctx;
    ctx->resp = this->shared_from_this();

    uv_write(&ctx->req, m_client,
             m_buffers.data(),
             m_buffers.size(),
             onWriteEnd);
}

void httpResp::onWriteEnd(uv_write_t *req, int status)
{
    if (status < 0)
    {
        std::cerr << "http response write error" << std::endl;
    } else
    {
        std::cout << "http response write success" << std::endl;
    }

    auto ctx = static_cast<WriteContext*>(req->data);
    if (ctx->resp->m_sendType == SendType::FILE)
    {
        delete ctx->resp->m_reader;
    }
    if (ctx->resp->m_onSent)
    {
        ctx->resp->m_onSent(ctx->resp.get());
    }

    delete ctx;
}

void httpResp::onSent(const std::function<void(httpResp*)>& cb)
{
    m_onSent = cb;
}

void httpResp::clearContent()
{
    m_body.clear();
    m_headers.clear();
    m_status = httpStatus::OK;
    m_sendType = SendType::NONE;
    m_filePath.clear();
}

void httpResp::sendErr()
{
    m_body.clear();
    m_headers.clear();
    setStatus(httpStatus::INTERNAL_SERVER_ERROR);
    sendStr(httpStatus_str(httpStatus::INTERNAL_SERVER_ERROR));
}
