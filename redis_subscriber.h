#include <chrono>
#include <format>
#include <hiredis/read.h>
#include <iostream>
#include <iterator>
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

#define CONTEXT_INIT_FAIL -1

#define EXCEPT_NONE  0
#define EXCEPT_PRINT 1
#define EXCEPT_THROW 2
#define EXCEPT_BOTH  3

#define PORT         6379
#define SRC_ADDR     "127.0.0.1"
#define CONN_TIME    1000
#define ALIVE_INTVL  30000


class RedisError : public std::runtime_error{
public:
    explicit RedisError(const std::string& err) : std::runtime_error(err) {}
};


class RedisBase{
public:
    using ConnectCallBack = std::function<void(int)>;
    using DisconnectCallBack = std::function<void(int)>;

    RedisBase(const std::shared_ptr<Logger>& l, ConnectCallBack con_cb, 
              DisconnectCallBack dcon_cb, const std::string& h, int p = PORT,
              const std::string& src_a = SRC_ADDR, int con_t = CONN_TIME,
              int al_int = ALIVE_INTVL);
    virtual ~RedisBase();


    
    void connect(); 
    std::atomic<bool> connected;
    //no throw guarantee: this func will throw exception only if
    //there is a connection/io issue (when the client can't get reply from server).
    //binary-safe: cmd is not binary-safe



private:
    std::shared_ptr<Logger> logger;
    
    redisOptions redis_opt;
    redisAsyncContext* redis_ctx;
    event_base* base;
    
    ConnectCallBack connect_callback;
    DisconnectCallBack disconnect_callback;

    
    std::string host, source_addr;
    int port;
    timeval connect_tv;
    int alive_interval;

    


    std::string proc_error(int flag, const std::string& error_pre);


};
