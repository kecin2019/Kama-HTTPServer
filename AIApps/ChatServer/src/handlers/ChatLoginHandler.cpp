#include "../include/handlers/ChatLoginHandler.h"

void ChatLoginHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
{
    // 处理登录请求
    // 验证 contentType
    auto contentType = req.getHeader("Content-Type");
    if (contentType.empty() || contentType != "application/json" || req.getBody().empty())
    {
        LOG_INFO << "content" << req.getBody();
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(0);
        resp->setBody("");
        return;
    }

    // 解析 JSON 请求体
    try
    {
        json parsed = json::parse(req.getBody());
        std::string username = parsed["username"];
        std::string password = parsed["password"];
        // 验证用户名和密码是否正确
        int userId = queryUserId(username, password);
        if (userId != -1)
        {
            // 获取会话对象
            auto session = server_->getSessionManager()->getSession(req, resp);

            // 存储用户ID、用户名和登录状态到会话中
            // 会话ID 作为键，用户ID、用户名和登录状态作为值
            session->setValue("userId", std::to_string(userId));
            session->setValue("username", username);
            session->setValue("isLoggedIn", "true");
            if (server_->onlineUsers_.find(userId) == server_->onlineUsers_.end() || server_->onlineUsers_[userId] == false)
            {
                {
                    std::lock_guard<std::mutex> lock(server_->mutexForOnlineUsers_);
                    server_->onlineUsers_[userId] = true;
                }

                // 用户登录成功，返回 200 成功响应
                // 构建成功响应 JSON 数据
                json successResp;
                successResp["success"] = true;
                successResp["userId"] = userId;
                std::string successBody = successResp.dump(4);

                resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
                resp->setCloseConnection(false);
                resp->setContentType("application/json");
                resp->setContentLength(successBody.size());
                resp->setBody(successBody);
                return;
            }
            else
            {
                // 用户已登录，返回 403 禁止访问错误
                // 构建禁止响应 JSON 数据
                json failureResp;
                failureResp["success"] = false;
                failureResp["error"] = "User already logged in";
                std::string failureBody = failureResp.dump(4);

                resp->setStatusLine(req.getVersion(), http::HttpResponse::k403Forbidden, "Forbidden");
                resp->setCloseConnection(true);
                resp->setContentType("application/json");
                resp->setContentLength(failureBody.size());
                resp->setBody(failureBody);
                return;
            }
        }
        else // 用户名或密码错误
        {
            // 构建错误响应 JSON 数据
            json failureResp;
            failureResp["status"] = "error";
            failureResp["message"] = "Invalid username or password";
            std::string failureBody = failureResp.dump(4);

            resp->setStatusLine(req.getVersion(), http::HttpResponse::k401Unauthorized, "Unauthorized");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(failureBody.size());
            resp->setBody(failureBody);
            return;
        }
    }
    catch (const std::exception &e)
    {
        // 处理 JSON 解析异常，返回 400 错误
        json failureResp;
        failureResp["status"] = "error";
        failureResp["message"] = e.what();
        std::string failureBody = failureResp.dump(4);

        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
        return;
    }
}

int ChatLoginHandler::queryUserId(const std::string &username, const std::string &password)
{
    // 检查用户名和密码是否正确
    // 执行查询语句
    std::string sql = "SELECT id FROM users WHERE username = ? AND password = ?";
    // std::vector<std::string> params = {username, password};
    auto res = mysqlUtil_.executeQuery(sql, username, password);
    if (res->next())
    {
        int id = res->getInt("id");
        return id;
    }
    // 查询结果为空，返回 -1 表示登录失败
    return -1;
}
