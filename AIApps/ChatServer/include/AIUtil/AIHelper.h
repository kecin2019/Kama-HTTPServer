#pragma once
#include <string>
#include <vector>
#include <utility>
#include <curl/curl.h>
#include <iostream>
#include <sstream>

#include "../../../../HttpServer/include/utils/JsonUtil.h"
#include "../../../../HttpServer/include/utils/MysqlUtil.h"

// 这边封装curl去访问对阿里的模型
class AIHelper
{
public:
    // 构造函数，初始化API Key
    AIHelper(const std::string &apiKey);

    // 设置默认模型
    void setModel(const std::string &modelName);

    // 添加一条消息
    void addMessage(int userId, const std::string &userName, bool is_user, const std::string &userInput);
    // 恢复一条消息
    void restoreMessage(const std::string &userInput, long long ms);

    // 发送聊天消息，返回AI的响应内容
    // messages: [{"role":"system","content":"..."}, {"role":"user","content":"..."}]
    std::string chat(int userId, std::string userName);

    // 可选：发送自定义请求体
    json request(const json &payload);

    std::vector<std::pair<std::string, long long>> GetMessages();

private:
    std::string escapeString(const std::string &input);
    // 加入到mysql的接口（提供加入到线程池的接口，线程池做异步mysql更新操作）
    // todo:
    void pushMessageToMysql(int userId, const std::string &userName, bool is_user, const std::string &userInput, long long ms);

    // 内部方法：执行curl请求，返回原始JSON
    json executeCurl(const json &payload);
    // curl 回调函数，把返回的数据写到 string buffer
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

private:
    std::string apiKey_;
    // 默认用通义千问
    std::string model_ = "qwen-plus";
    // 对应地址
    std::string apiUrl_ = "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions";

    // 一个用户针对一个AIHelper，messages存放用户的历史对话
    // 偶数下标代表用户的信息，奇数下标是ai返回的内容
    // 后者代表时间戳
    std::vector<std::pair<std::string, long long>> messages;

    // http::MysqlUtil mysqlUtil_;
};
