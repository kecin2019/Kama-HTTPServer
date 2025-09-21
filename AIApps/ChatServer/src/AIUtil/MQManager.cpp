#include"../include/AIUtil/MQManager.h"

// ------------------- MQManager -------------------
MQManager::MQManager(size_t poolSize)
    : poolSize_(poolSize), counter_(0) {
    for (size_t i = 0; i < poolSize_; ++i) {
        auto conn = std::make_shared<MQConn>();
        // 保留 Create
        conn->channel = AmqpClient::Channel::Create("localhost", 5672, "guest", "guest", "/");
        // 这里不重复声明队列，避免 exclusive use
        pool_.push_back(conn);
    }
}

void MQManager::publish(const std::string& queue, const std::string& msg) {
    size_t index = counter_.fetch_add(1) % poolSize_;
    auto& conn = pool_[index];

    std::lock_guard<std::mutex> lock(conn->mtx);
    auto message = AmqpClient::BasicMessage::Create(msg);
    conn->channel->BasicPublish("", queue, message);
}

// ------------------- RabbitMQThreadPool -------------------

void RabbitMQThreadPool::start() {
    for (int i = 0; i < thread_num_; ++i) {
        workers_.emplace_back(&RabbitMQThreadPool::worker, this, i);
    }
}

void RabbitMQThreadPool::shutdown() {
    stop_ = true;
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
}

void RabbitMQThreadPool::worker(int id) {
    try {
        // 每个线程独立 channel
        auto channel = AmqpClient::Channel::Create(rabbitmq_host_, 5672, "guest", "guest", "/");
        // 声明队列（非 exclusive）
        channel->DeclareQueue(queue_name_, false, true, false, false);
        //防止出现：channel error: 403: AMQP_BASIC_CONSUME_METHOD caused: ACCESS_REFUSED - queue 
        // 'sql_queue' in vhost '/' in exclusive use
        //std::string consumer_tag = channel->BasicConsume(queue_name_, "");
        std::string consumer_tag = channel->BasicConsume(queue_name_, "", true, false, false);

        channel->BasicQos(consumer_tag, 1); // 每个线程一次只处理一条消息

        while (!stop_) {
            AmqpClient::Envelope::ptr_t env;
            bool ok = channel->BasicConsumeMessage(consumer_tag, env, 500); // 500ms 超时
            if (ok && env) {
                std::string msg = env->Message()->Body();
                handler_(msg);          // 用户处理消息
                channel->BasicAck(env); // 确认消息
            }
        }

        channel->BasicCancel(consumer_tag);
    }
    catch (const std::exception& e) {
        std::cerr << "Thread " << id << " exception: " << e.what() << std::endl;
    }
}
