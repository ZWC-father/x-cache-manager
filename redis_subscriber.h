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

class SubManager;

class RedisError : public std::runtime_error{
public:
    explicit RedisError(const std::string& err) : std::runtime_error(err) {}
};


class RedisSub{
public:
    using RedisCallback = redisCallbackFn;
    RedisSub(SubManager* manager, const std::shared_ptr<Logger>& l,
             const std::string& h, int p, const std::string& src_a, int con_t);
    ~RedisSub();

    int connect();
    void disconnect();
    void free();

    void run();
    int subscribe(const std::string& channel, RedisCallback callback,
                  bool psub = false);

private:
    std::string host, source_addr;
    int port;
    timeval connect_tv;
    
    std::shared_ptr<Logger> logger;
    event_base* base;
    event* event_guard;
    redisOptions redis_opt;
    
    SubManager* manager;
    
    static void connect_cb(const redisAsyncContext* redis_ctx, int res);
    static void disconnect_cb(const redisAsyncContext* redis_ctx, int res);
    static void dummy_cb(evutil_socket_t fd, short what, void *arg) {}
};


class SubManager{
public:
    SubManager(const std::shared_ptr<Logger>& l, const std::string& host,
               int port = PORT, const std::string& source_addr = SRC_ADDR,
               int connect_timeout = CONN_TIME, int max_retries = RETRY_MAX,
               int retry_interval = RETRY_INTVL);
    ~SubManager();
    
    redisAsyncContext* redis_ctx;

    void init();
    void uninit(); 

    void connect_callback(int res, const std::string& msg);
    void disconnect_callback(int res, const std::string& msg);

private:
    std::string host, source_addr;
    int port;
    int connect_timeout, max_tries;
    std::chrono::milliseconds retry_interval;
    int try_count;

    std::shared_ptr<Logger> logger;
    std::mutex manager_lock;
    std::condition_variable cv;
    std::promise<bool> prom, except_prom;
    std::future<bool> fut, except_fut;
    std::atomic<bool> pending, stop_signal;

    RedisSub* redis;
    std::thread connection_loop;
    std::thread event_loop;

    void do_connect();
    void connection_manager();
};
