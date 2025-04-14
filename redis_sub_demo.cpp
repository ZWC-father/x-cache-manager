#include <iostream>
#include <stdexcept>
#include <string>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>
#include <event2/event.h>

// 自定义异常类型
class RedisException : public std::runtime_error {
public:
    explicit RedisException(const std::string& msg) : std::runtime_error(msg) {}
};

// 自定义回调函数类型
// 回调函数的原型和 hiredis async API 保持一致：
// void callback(redisAsyncContext *c, void *r, void *privdata)
using RedisCallback = void(*)(redisAsyncContext*, void*, void*);

// 订阅器封装类
class RedisSubscriber {
public:
    // 构造函数：连接到 Redis 服务器，并创建 event_base
    RedisSubscriber(const std::string &host, int port) : _base(nullptr), _ac(nullptr) {
        // 创建 libevent 事件循环
        _base = event_base_new();
        if (!_base) {
            throw RedisException("Failed to create event_base");
        }
        // 建立异步连接
        _ac = redisAsyncConnect(host.c_str(), port);
        if (!_ac || _ac->err) {
            std::string errMsg = _ac ? _ac->errstr : "Cannot allocate async context";
            if (_ac) redisAsyncFree(_ac);
            event_base_free(_base);
            throw RedisException("Async connection error: " + errMsg);
        }
        // 绑定 hiredis 上下文到 libevent
        if (redisLibeventAttach(_ac, _base) != REDIS_OK) {
            redisAsyncFree(_ac);
            event_base_free(_base);
            throw RedisException("Failed to attach redisAsyncContext to event_base");
        }
        // 注册连接和断开回调（可选）
        redisAsyncSetConnectCallback(_ac, connectCallback);
        redisAsyncSetDisconnectCallback(_ac, disconnectCallback);
    }

    // 析构函数，释放资源
    ~RedisSubscriber() {
        // 如果连接还在，主动断开
        if (_ac) {
            // 这里调用 disconnect 并等待连接关闭
            redisAsyncDisconnect(_ac);
            _ac = nullptr;
        }
        if (_base) {
            event_base_free(_base);
            _base = nullptr;
        }
    }

    // 订阅指定频道或模式。psubscribe 为 true 表示使用模式订阅
    void subscribe(const std::string &channel, RedisCallback callback, void *privdata = nullptr, bool psubscribe = false) {
        if (!_ac) {
            throw RedisException("Not connected");
        }
        // 构造命令字符串
        const char *cmd = psubscribe ? "PSUBSCRIBE %s" : "SUBSCRIBE %s";
        int ret = redisAsyncCommand(_ac, callback, privdata, cmd, channel.c_str());
        if (ret != REDIS_OK) {
            throw RedisException("Failed to subscribe to channel " + channel + ": " + _ac->errstr);
        }
    }

    // 启动事件循环，阻塞等待事件
    void run() {
        if (!_base) {
            throw RedisException("Event base is null");
        }
        event_base_dispatch(_base);
    }

private:
    // 连接成功回调
    static void connectCallback(const redisAsyncContext *c, int status) {
        if (status != REDIS_OK) {
            std::cerr << "Connect callback error: " << c->errstr << std::endl;
        } else {
            std::cout << "Connected to Redis (async)." << std::endl;
        }
    }

    // 断开连接回调
    static void disconnectCallback(const redisAsyncContext *c, int status) {
        if (status != REDIS_OK) {
            std::cerr << "Disconnect callback error: " << c->errstr << std::endl;
        } else {
            std::cout << "Disconnected from Redis (async)." << std::endl;
        }
    }

private:
    event_base* _base;
    redisAsyncContext* _ac;
};

// 自定义的订阅消息回调函数示例
void myMessageCallback(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = static_cast<redisReply*>(r);
    if (!reply) {
        // 如果为 NULL，可能是连接已关闭，直接返回
        return;
    }
    // 通常 reply->type == REDIS_REPLY_ARRAY，包含消息详细信息
    std::cout << "Received message:" << std::endl;
    for (size_t i = 0; i < reply->elements; i++) {
        if (reply->element[i]->type == REDIS_REPLY_STRING) {
            std::cout << "  " << reply->element[i]->str << std::endl;
        }
    }
    // 注意：不调用 freeReplyObject(reply) —— hiredis 会自动释放异步回复
}

int main() {
    try {
        // 构造订阅对象，连接到本地 Redis
        RedisSubscriber subscriber("127.0.0.1", 6379);
        
        // 设置通知（注意：确保 Redis 服务器开启了 keyspace notifications，如 notify-keyspace-events Ex）
        // 订阅键过期事件（假设数据库 0）
        subscriber.subscribe("__keyevent@0__:expired", myMessageCallback);
        // 订阅键被驱逐事件
        subscriber.subscribe("__keyevent@0__:evicted", myMessageCallback);
        
        // 你还可以继续订阅其他频道或模式
        // subscriber.subscribe("someOtherChannel", myMessageCallback);

        // 启动事件循环
        subscriber.run();
    } catch(const std::exception &ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
        return -1;
    }
    return 0;
}

