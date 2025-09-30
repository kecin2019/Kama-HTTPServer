#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include "../include/ChatServer.h"

const std::string RABBITMQ_HOST = "localhost";
const std::string QUEUE_NAME = "sql_queue";
const int THREAD_NUM = 2;

void executeMysql(const std::string sql)
{
    http::MysqlUtil mysqlUtil_;
    mysqlUtil_.executeUpdate(sql);
}

int main(int argc, char *argv[])
{
    LOG_INFO << "pid = " << getpid();
    std::string serverName = "ChatServer";
    int port = 80;
    // 参数解析
    int opt;
    const char *str = "p:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p':
        {
            port = atoi(optarg);
            break;
        }
        default:
            break;
        }
    }
    muduo::Logger::setLogLevel(muduo::Logger::WARN);
    ChatServer server(port, serverName);
    server.setThreadNum(4);
    // 等待 2 秒，确保 RabbitMQ 队列已创建
    std::this_thread::sleep_for(std::chrono::seconds(2));
    // 初始化 chat_message 表，创建 chatInformation 映射
    server.initChatMessage();

    // 初始化 RabbitMQ 线程池，用于处理 SQL 队列中的消息
    // 每个线程从队列中获取一条消息并执行
    RabbitMQThreadPool pool(RABBITMQ_HOST, QUEUE_NAME, THREAD_NUM, executeMysql);
    pool.start();

    server.start();
}
