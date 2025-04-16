#include <atomic>
#include <chrono>
#include <condition_variable>
#include <format>
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

#include "logger.h"
#include "logging_zones.h"

#define PORT         6379
#define SRC_ADDR     "127.0.0.1"
#define CONN_TIME    1000
#define RETRY_INTVL  100

class RedisError : public std::runtime_error{
public:
    explicit RedisError(const std::string& err) : std::runtime_error(err) {}
};

class SubManager{
public:
    SubManager(const std::shared_ptr<Logger>& l, const std::string& host,
               int port = PORT, const std::string& source_addr = SRC_ADDR,
               int connect_timeout = CONN_TIME, int retry_interval = RETRY_INTVL);
    ~SubManager();

    void init();
    void run(); 

    void connect_callback(int res, const std::string& msg);
    void disconnect_callback(int res, const std::string& msg);

private:
    std::string host, source_addr;
    int port, connect_timeout;
    std::chrono::milliseconds retry_interval;
    
    redisAsyncContext* redis_ctx;

    std::shared_ptr<Logger> logger;
    std::thread connection_manager;
    std::thread event_loop;

    std::atomic<int> connection_status;

};

class RedisSub{
public:
    RedisSub(SubManager* manager, redisAsyncContext** ctx, const std::shared_ptr<Logger>& l,
             const std::string& h, int p, const std::string& src_a, int con_t);
    ~RedisSub();

    bool connect();
    void disconnect();
    void free();
    void set_null();

    void run();

private:
    struct External{
        redisAsyncContext* redis_ctx;
        SubManager* manager;
    };
    
    std::string host, source_addr;
    int port;
    timeval connect_tv;
    
    std::shared_ptr<Logger> logger;
    event_base* base;
    redisOptions redis_opt;
    redisAsyncContext** redis_ctx;
    External* external;
    
    static void connect_cb(const redisAsyncContext* redis_ctx, int res);
    static void disconnect_cb(const redisAsyncContext* redis_ctx, int res);


};


