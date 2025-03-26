#include <chrono>
#include <format>
#include <hiredis/read.h>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <mutex>
#include <ratio>
#include <stdexcept>
#include <thread>
#include <functional>
#include <utility>
#include <hiredis/hiredis.h>

#define EXCEPT_THROW 0
#define EXCEPT_NO_THROW 1

#define PORT 6379
#define SRC_ADDR "127.0.0.1"
#define CONN_TIME 1000
#define CMD_TIME 1000
#define ALIVE_INTVL 30000
#define RETRY_INTVL 200
#define MAX_RETRY 3

class RedisError : public std::runtime_error{
public:
    explicit RedisError(const std::string& err) : std::runtime_error(err) {}
};

class RedisBase{
public:
    RedisBase(const std::string& h, int p, const std::string& src_a, int con_t,
              int cmd_t, int al_int, int rt_int, int max_rt);
    ~RedisBase();
    
    void connect();
    void disconnect();
    void reconnect();

   
    template<typename... Args>
    void command()

private:
    std::thread connect_daemo;
    redisOptions redis_opt;
    redisContext* redis_ctx;
    
    std::string host, source_addr;
    int port;
    timeval connect_tv, command_tv;
    int alive_interval, max_retries;
    std::chrono::milliseconds retry_interval;

    void proc_error(bool no_throw, const std::string& error_pre);

    REDIS_REPLY_NIL
    template<typename... Args>
    redisReply* redis_command_impl(const char* cmd, const Args&&... args){
        redisReply* reply;
        for(int i = 1; i <= max_retries; i++){
            reply = redisCommand(redis_ctx, cmd,
                                 std::forward<Args>(args)...);
            if(i == max_retries)return reply;
            if(reply == NULL){
                proc_error(EXCEPT_NO_THROW,
                           "command error: "); 
                if(redis_ctx->err == REDIS_ERR_EOF ||
                   redis_ctx->err == REDIS_ERR_IO){
                    std::cerr << "note: maybe connection problem" << std::endl;
                    reconnect();
                }
            }else return reply;
            std::this_thread::sleep_for(retry_interval);
            std::cerr << "command (" << cmd << ") retry: #" << i << std::endl;
            
        }
    }
   
};
