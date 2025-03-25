#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <functional>
#include <utility>
#include <hiredis/hiredis.h>

#define PORT 6379
#define SRC_ADDR "127.0.0.1"
#define CONN_TIME 1000
#define CMD_TIME 1000
#define CHK_INTVL 30000
#define MAX_RETRY 3

class RedisError : public std::runtime_error{
    explicit RedisError(const std::string& err) : std::runtime_error(err) {}
};

class RedisBase{
public:
    RedisBase(const std::string& host, int port, const std::string& bind_addr, 
              int connect_timeout, int command_timeout, int alive_interval,
              int max_retries);
    ~RedisBase();
    bool connect();
    bool disconnect();


private:
    std::thread connect_daemo;
    redisOptions redis_opt;
    redisContext* redis_ctx;
    
    std::string host;
    std::string source_addr;
    int port;
    timeval connect_tv, command_tv;
    int alive_interval, max_retries;
    bool is_connected();
    void re_connect();
    void do_connect();
    
   
};
