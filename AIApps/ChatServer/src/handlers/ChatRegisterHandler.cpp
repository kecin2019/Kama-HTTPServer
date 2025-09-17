#include "../include/handlers/ChatRegisterHandler.h"


void ChatRegisterHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    // 解析body(json格式)
    json parsed = json::parse(req.getBody());
    std::string username = parsed["username"];
    std::string password = parsed["password"];

    // 判断用户是否已经存在，如果存在则注册失败
    int userId = insertUser(username, password);
    if (userId != -1)
    {
        // 插入成功
        // 封装成功响应
        json successResp;
        successResp["status"] = "success";
        successResp["message"] = "Register successful";
        successResp["userId"] = userId;
        std::string successBody = successResp.dump(4);

        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(successBody.size());
        resp->setBody(successBody);
    }
    else
    {
        // 插入失败
        json failureResp;
        failureResp["status"] = "error";
        failureResp["message"] = "username already exists";
        std::string failureBody = failureResp.dump(4);

        resp->setStatusLine(req.getVersion(), http::HttpResponse::k409Conflict, "Conflict");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
    }
}

int ChatRegisterHandler::insertUser(const std::string& username, const std::string& password)
{
    // 判断用户是否存在，如果存在则返回-1，否则返回用户id
    if (!isUserExist(username))
    {
        // 用户不存在，插入用户
        std::string sql = "INSERT INTO users (username, password) VALUES ('" + username + "', '" + password + "')";
        mysqlUtil_.executeUpdate(sql);
        std::string sql2 = "SELECT id FROM users WHERE username = '" + username + "'";
        auto res = mysqlUtil_.executeQuery(sql2);
        /*
        std::string sql2 = "SELECT id, username, is_user, content, ts FROM chat_message ORDER BY ts ASC, id ASC";
        auto res = mysqlUtil_.executeQuery(sql2);
        while (res.next()) {
            long long user_id = 0;
            std::string username, content;
            long long ts = 0;
            int is_user = 1;

            try {
                user_id = res.getInt64("id");
                username = res.getString("username");
                content = res.getString("content");
                ts = res.getInt64("ts");
                is_user = res.getInt("is_user");
                std::cout << "user_id: " << user_id
                    << ", username: " << username
                    << ", content: " << content
                    << ", ts: " << ts
                    << ", is_user: " << is_user
                    << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Failed to read row: " << e.what() << std::endl;
                continue; // 跳过异常行
            }
        }
        */
        if (res->next())
        {
            return res->getInt("id");
        }
    }
    return -1;
}

bool ChatRegisterHandler::isUserExist(const std::string& username)
{
    std::string sql = "SELECT id FROM users WHERE username = '" + username + "'";
    auto res = mysqlUtil_.executeQuery(sql);
    if (res->next())
    {
        return true;
    }
    return false;
}