#include "../include/handlers/ChatLoginHandler.h"
#include "../include/handlers/ChatRegisterHandler.h"
#include "../include/handlers/ChatLogoutHandler.h"
#include "../include/handlers/ChatHandler.h"
#include "../include/handlers/ChatEntryHandler.h"
#include "../include/handlers/ChatSendHandler.h"
#include "../include/handlers/AIMenuHandler.h"
#include "../include/handlers/AIUploadSendHandler.h"
#include "../include/handlers/AIUploadHandler.h"
#include "../include/handlers/ChatHistoryHandler.h"

#include "../include/ChatServer.h"
#include "../../../HttpServer/include/http/HttpRequest.h"
#include "../../../HttpServer/include/http/HttpResponse.h"
#include "../../../HttpServer/include/http/HttpServer.h"

using namespace http;

ChatServer::ChatServer(int port,
                       const std::string &name,
                       muduo::net::TcpServer::Option option)
    : httpServer_(port, name, option)
{
    initialize();
}

void ChatServer::initialize()
{
    std::cout << "ChatServer initialize start  ! " << std::endl;
    http::MysqlUtil::init("tcp://127.0.0.1:3306", "root", "123456", "ChatHttpServer", 5);
    // 初始化会话管理
    initializeSession();
    // 初始化中间件
    initializeMiddleware();
    // 初始化路由
    initializeRouter();
}

void ChatServer::initChatMessage()
{
    // 从MySQL数据库读取chat_message表中的数据，根据user_id将消息分组存储到chatInformation中
    std::cout << "initChatMessage start ! " << std::endl;
    readDataFromMySQL();
    std::cout << "initChatMessage success ! " << std::endl;
}

void ChatServer::readDataFromMySQL()
{

    const char *apiKey = std::getenv("DASHSCOPE_API_KEY");
    if (!apiKey)
    {
        std::cerr << "Error: DASHSCOPE_API_KEY not found in environment!" << std::endl;
        return;
    }

    // SQL 查询语句，按时间戳和ID排序
    std::string sql = "SELECT id, username, is_user, content, ts FROM chat_message ORDER BY ts ASC, id ASC";

    sql::ResultSet *res;
    try
    {
        res = mysqlUtil_.executeQuery(sql);
    }
    catch (const std::exception &e)
    {
        std::cerr << "MySQL query failed: " << e.what() << std::endl;
        return;
    }

    while (res->next())
    {
        long long user_id = 0;
        std::string username, content;
        long long ts = 0;
        int is_user = 1;

        try
        {
            user_id = res->getInt64("id");
            username = res->getString("username");
            content = res->getString("content");
            ts = res->getInt64("ts");
            is_user = res->getInt("is_user");
        }
        catch (const std::exception &e)
        {
            std::cerr << "Failed to read row: " << e.what() << std::endl;
            continue; // 跳过当前行
        }

        // 找到或创建对应的 AIHelper 实例
        std::shared_ptr<AIHelper> helper;
        auto it = chatInformation.find(user_id);
        if (it == chatInformation.end())
        {
            helper = std::make_shared<AIHelper>(apiKey);
            chatInformation[user_id] = helper;
        }
        else
        {
            helper = it->second;
        }

        // 恢复消息到 AIHelper 实例
        helper->restoreMessage(content, ts);
    }

    std::cout << "readDataFromMySQL finished" << std::endl;
}

void ChatServer::setThreadNum(int numThreads)
{
    httpServer_.setThreadNum(numThreads);
}

void ChatServer::start()
{
    httpServer_.start();
}

void ChatServer::initializeRouter()
{
    // 注册URL路径和对应的处理程序
    // 首页
    httpServer_.Get("/", std::make_shared<ChatEntryHandler>(this));
    httpServer_.Get("/entry", std::make_shared<ChatEntryHandler>(this));
    // 登录
    httpServer_.Post("/login", std::make_shared<ChatLoginHandler>(this));
    // 注册
    httpServer_.Post("/register", std::make_shared<ChatRegisterHandler>(this));
    // 注销
    httpServer_.Post("/user/logout", std::make_shared<ChatLogoutHandler>(this));
    // 聊天页面
    httpServer_.Get("/chat", std::make_shared<ChatHandler>(this));
    // 发送消息
    httpServer_.Post("/chat/send", std::make_shared<ChatSendHandler>(this));
    // 菜单页面
    httpServer_.Get("/menu", std::make_shared<AIMenuHandler>(this));
    // 上传文件页面
    httpServer_.Get("/upload", std::make_shared<AIUploadHandler>(this));
    // 上传文件
    httpServer_.Post("/upload/send", std::make_shared<AIUploadSendHandler>(this));
    // 聊天历史记录
    httpServer_.Post("/chat/history", std::make_shared<ChatHistoryHandler>(this));
}

void ChatServer::initializeSession()
{
    // 会话存储
    auto sessionStorage = std::make_unique<http::session::MemorySessionStorage>();
    // 会话管理器
    auto sessionManager = std::make_unique<http::session::SessionManager>(std::move(sessionStorage));
    // 用户会话管理器
    setSessionManager(std::move(sessionManager));
}

void ChatServer::initializeMiddleware()
{
    // 跨域中间件
    auto corsMiddleware = std::make_shared<http::middleware::CorsMiddleware>();
    // 添加跨域中间件
    httpServer_.addMiddleware(corsMiddleware);
}

void ChatServer::packageResp(const std::string &version,
                             http::HttpResponse::HttpStatusCode statusCode,
                             const std::string &statusMsg,
                             bool close,
                             const std::string &contentType,
                             int contentLen,
                             const std::string &body,
                             http::HttpResponse *resp)
{
    if (resp == nullptr)
    {
        LOG_ERROR << "Response pointer is null";
        return;
    }

    try
    {
        resp->setVersion(version);
        resp->setStatusCode(statusCode);
        resp->setStatusMessage(statusMsg);
        resp->setCloseConnection(close);
        resp->setContentType(contentType);
        resp->setContentLength(contentLen);
        resp->setBody(body);

        LOG_INFO << "Response packaged successfully";
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Error in packageResp: " << e.what();
        // 打包失败时，设置默认的错误状态码和消息
        resp->setStatusCode(http::HttpResponse::k500InternalServerError);
        resp->setStatusMessage("Internal Server Error");
        resp->setCloseConnection(true);
    }
}
