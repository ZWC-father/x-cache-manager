#include <atomic>
#include <chrono>
#include <condition_variable>
#include <format>
#include <future>
#include <hiredis/read.h>
#include <iostream>
#include <iterator>
#include <shared_mutex>
#include <sys/socket.h>
#include <variant>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <mutex>
#include <ratio>
#include <stdexcept>
#include <thread>
#include <functional>
#include <utility>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>
#include <event2/event.h>
#include <vector>

#include "logger.h"
#include "logging_zones.h"

#define CHANNEL "__keyevent@0__:evicted"
#define PSUB false

#define PORT         6379
#define SRC_ADDR     "127.0.0.1"
#define CONN_TIME    1000

class SubManager;

#ifndef REDIS_ERROR
#define REDIS_ERROR
class RedisError : public std::runtime_error{
public:
    explicit RedisError(const std::string& err) : std::runtime_error(err) {}
};
#endif


class RedisSub{
public:
    using RedisCallback = redisCallbackFn;
    RedisSub(SubManager* manager, const std::shared_ptr<Logger>& l,
             const std::string& h, int p, const std::string& src_a, int con_t);
    ~RedisSub();

    int connect();
    void disconnect();

    void run();
    int subscribe(const std::string& channel, RedisCallback callback,
                  bool psub = false);

private:
    std::string host, source_addr;
    int port;
    timeval connect_tv;
    
    std::shared_ptr<Logger> logger;
    event_base* base;
    redisOptions redis_opt;
    
    SubManager* manager;
    
    void init();
    void uninit();
    void free();
    
    static void connect_cb(const redisAsyncContext* redis_ctx, int res);
    static void disconnect_cb(const redisAsyncContext* redis_ctx, int res);
};


class SubManager{
public:
    struct RedisReplyStringArray {std::vector<std::string> vec;};
    struct RedisReplyStatus {std::string str;};
    struct RedisReplyError {std::string str;};
    using Reply = std::variant<RedisReplyStringArray,
                               RedisReplyStatus, RedisReplyError>;
    using SubCallback = std::function<void(Reply)>;
    
    SubManager(SubCallback sub_callback, const std::shared_ptr<Logger>& logger,
               const std::string& host, int port = PORT,
               const std::string& source_addr = SRC_ADDR,
               int connect_timeout = CONN_TIME);
    
    std::shared_ptr<Logger> logger;
    redisAsyncContext* redis_ctx;
    std::atomic<int> status;

    void init();
    void uninit();
    void connect();
    int subscribe();
    //you must call disconnect() and wait connect() for exit

    void connect_callback(int res, const std::string& msg);
    void disconnect_callback(int res, const std::string& msg);
    static void command_callback(redisAsyncContext* ctx,
                                 void* reply, void* privdata);

private:
    std::string host, source_addr;
    int port, connect_timeout;
    SubCallback sub_callback;

    std::condition_variable cv;
    std::mutex connect_lock;
    std::atomic<bool> running;

    
    //-1 = failure, 0 = nothing, 1 = connected

    RedisSub* redis;

    void free();
};
