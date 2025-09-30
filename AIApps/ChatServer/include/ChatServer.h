#pragma once

#include <atomic>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <mutex>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "../../../HttpServer/include/http/HttpServer.h"
#include "../../../HttpServer/include/utils/MysqlUtil.h"
#include "../../../HttpServer/include/utils/FileUtil.h"
#include "../../../HttpServer/include/utils/JsonUtil.h"
#include "AIUtil/AIHelper.h"
#include "AIUtil/ImageRecognizer.h"
#include "AIUtil/base64.h"
#include "AIUtil/MQManager.h"

class ChatLoginHandler;
class ChatRegisterHandler;
class ChatLogoutHandler;
class ChatHandler;
class ChatEntryHandler;
class ChatSendHandler;
class ChatHistoryHandler;

class AIMenuHandler;
class AIUploadHandler;
class AIUploadSendHandler;

class ChatServer
{
public:
	ChatServer(int port,
			   const std::string &name,
			   muduo::net::TcpServer::Option option = muduo::net::TcpServer::kNoReusePort);

	void setThreadNum(int numThreads);
	void start();
	void initChatMessage();

private:
	friend class ChatLoginHandler;
	friend class ChatRegisterHandler;
	friend ChatLogoutHandler;
	friend class ChatHandler;
	friend class ChatEntryHandler;
	friend class ChatSendHandler;
	friend class AIMenuHandler;
	friend class AIUploadHandler;
	friend class AIUploadSendHandler;
	friend class ChatHistoryHandler;

private:
	void initialize();
	void initializeSession();
	void initializeRouter();
	void initializeMiddleware();

	void readDataFromMySQL();

	void packageResp(const std::string &version, http::HttpResponse::HttpStatusCode statusCode,
					 const std::string &statusMsg, bool close, const std::string &contentType,
					 int contentLen, const std::string &body, http::HttpResponse *resp);

	void setSessionManager(std::unique_ptr<http::session::SessionManager> manager)
	{
		httpServer_.setSessionManager(std::move(manager));
	}
	http::session::SessionManager *getSessionManager() const
	{
		return httpServer_.getSessionManager();
	}
	// 聊天服务器实例
	http::HttpServer httpServer_;
	// 数据库工具类实例
	http::MysqlUtil mysqlUtil_;
	// 记录在线用户的映射表，用户ID到是否在线的布尔值
	std::unordered_map<int, bool> onlineUsers_;
	std::mutex mutexForOnlineUsers_;
	// 每个用户的聊天信息映射表，用户ID到AIHelper智能指针
	// 说明：每个用户在聊天过程中可能会有多个AIHelper实例，每个实例对应一个AI模型
	// 注意：chatInformation[key]中的AIHelper实例是动态创建的，需要在使用前检查是否为空
	// 警告：chatInformation[key]中的AIHelper实例在聊天过程中可能会被修改，需要注意线程安全问题
	std::unordered_map<int, std::shared_ptr<AIHelper>> chatInformation;
	std::mutex mutexForChatInformation;

	std::unordered_map<int, std::shared_ptr<ImageRecognizer>> ImageRecognizerMap;
	std::mutex mutexForImageRecognizerMap;
};
