#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include"../include/ChatServer.h"

const std::string RABBITMQ_HOST = "localhost";
const std::string QUEUE_NAME = "sql_queue";
const int THREAD_NUM = 2;

void executeMysql(const std::string sql) {
    http::MysqlUtil mysqlUtil_;
    mysqlUtil_.executeUpdate(sql);
}


int main(int argc, char* argv[]) {
	LOG_INFO << "pid = " << getpid();
	std::string serverName = "ChatServer";
	int port = 80;
    // 参数解析
    int opt;
    const char* str = "p:";
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
    //这边一定要进行睡眠操作，如果放在ChatServer构造函数中初始会出现卡死现象
    std::this_thread::sleep_for(std::chrono::seconds(2));
    //初始化chat_message表到chatInformation中
    server.initChatMessage();    

    // 初始化消费队列的线程池，传入处理函数（这边所有线程都做统一的处理函数逻辑）
    //如果要做到协程库那种每个线程做不同的任务，那么也可以再封一层任务类，线程拿取任务类中的函数进行执行操作
    RabbitMQThreadPool pool(RABBITMQ_HOST, QUEUE_NAME, THREAD_NUM, executeMysql);
    pool.start();

    server.start();
}
