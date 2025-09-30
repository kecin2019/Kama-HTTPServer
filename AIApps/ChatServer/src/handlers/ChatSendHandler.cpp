#include "../include/handlers/ChatSendHandler.h"

void ChatSendHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
{
    try
    {
        // 检查用户是否已登录
        auto session = server_->getSessionManager()->getSession(req, resp);
        LOG_INFO << "session->getValue(\"isLoggedIn\") = " << session->getValue("isLoggedIn");
        if (session->getValue("isLoggedIn") != "true")
        {
            // 用户未登录，返回未授权错误响应
            json errorResp;
            errorResp["status"] = "error";
            errorResp["message"] = "Unauthorized";
            std::string errorBody = errorResp.dump(4);

            server_->packageResp(req.getVersion(), http::HttpResponse::k401Unauthorized,
                                 "Unauthorized", true, "application/json", errorBody.size(),
                                 errorBody, resp);
            return;
        }

        // 从会话中获取用户ID和用户名
        int userId = std::stoi(session->getValue("userId"));
        std::string username = session->getValue("username");

        std::shared_ptr<AIHelper> AIHelperPtr;
        {
            std::lock_guard<std::mutex> lock(server_->mutexForChatInformation);
            if (server_->chatInformation.find(userId) == server_->chatInformation.end())
            {
                // 该用户还未初始化 AIHelper，根据环境变量初始化
                const char *apiKey = std::getenv("DASHSCOPE_API_KEY");
                if (!apiKey)
                {
                    std::cerr << "Error: DASHSCOPE_API_KEY not found in environment!" << std::endl;
                    return;
                }
                // 初始化新的 AIHelper 并存储到 chatInformation 中
                server_->chatInformation.emplace(
                    userId,
                    std::make_shared<AIHelper>(apiKey));
            }
            AIHelperPtr = server_->chatInformation[userId];
        }

        std::string userQuestion;
        auto body = req.getBody();
        if (!body.empty())
        {
            auto j = json::parse(body);
            if (j.contains("question"))
                userQuestion = j["question"];
        }
        // int userId, const std::string& userName, bool is_user, const std::string& userInput
        AIHelperPtr->addMessage(userId, username, true, userQuestion);

        std::string aiInformation = AIHelperPtr->chat(userId, username);
        json successResp;
        successResp["success"] = true;
        successResp["Information"] = aiInformation;
        std::string successBody = successResp.dump(4);

        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(successBody.size());
        resp->setBody(successBody);
        return;
    }
    catch (const std::exception &e)
    {
        // 处理异常情况，返回错误响应
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
