#include "../include/handlers/ChatLogoutHandler.h"

void ChatLogoutHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
{
    auto contentType = req.getHeader("Content-Type");
    if (contentType.empty() || contentType != "application/json" || req.getBody().empty())
    {
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(0);
        resp->setBody("");
        return;
    }

    // JSON 解析请求体 Try-Catch 异常处理
    try
    {
        // 获取会话对象
        auto session = server_->getSessionManager()->getSession(req, resp);
        // 从会话中获取用户ID
        int userId = std::stoi(session->getValue("userId"));
        // 清除会话中的所有值
        session->clear();
        // 销毁会话
        server_->getSessionManager()->destroySession(session->getId());

        json parsed = json::parse(req.getBody());

        { // 从在线用户列表中移除用户
            std::lock_guard<std::mutex> lock(server_->mutexForOnlineUsers_);
            server_->onlineUsers_.erase(userId);
        }

        // 构建成功响应体
        json response;
        response["message"] = "logout successful";
        std::string responseBody = response.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(responseBody.size());
        resp->setBody(responseBody);
    }
    catch (const std::exception &e)
    {
        // 处理 JSON 解析异常，返回错误响应
        json failureResp;
        failureResp["status"] = "error";
        failureResp["message"] = e.what();
        std::string failureBody = failureResp.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
    }
}