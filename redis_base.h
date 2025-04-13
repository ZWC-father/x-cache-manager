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

#include "logger.h"
#include "logging_zones.h"

#define EXCEPT_NONE  0
#define EXCEPT_PRINT 1
#define EXCEPT_THROW 2
#define EXCEPT_BOTH  3

#define PORT         6379
#define SRC_ADDR     "127.0.0.1"
#define CONN_TIME    1000
#define CMD_TIME     1000
#define ALIVE_INTVL  30000
#define RETRY_INTVL  200
#define MAX_RETRY    3


class RedisError : public std::runtime_error{
public:
    explicit RedisError(const std::string& err) : std::runtime_error(err) {}
};

struct redisReplyDelete{
    void operator()(redisReply* reply) const{
        if(reply != NULL){
            freeReplyObject(reply);
        }
    }
};

class RedisBase{
public:
    using Reply = std::unique_ptr<redisReply, redisReplyDelete>;

    RedisBase(const std::shared_ptr<Logger>& l, const std::string& h, int p = PORT,
              const std::string& src_a = SRC_ADDR, int con_t = CONN_TIME,
              int cmd_t = CMD_TIME, int al_int = ALIVE_INTVL,
              int rt_int = RETRY_INTVL, int max_rt = MAX_RETRY);
    virtual ~RedisBase();
 
    void connect();
    void disconnect();
    void reconnect();
    bool connected;
    //no throw guarantee: this func will throw exception only if
    //there is a connection/io issue (when the client can't get reply from server).
    //binary-safe: cmd is not binary-safe

    template<typename... Args>
    Reply command_single(const char* cmd, Args&&... args){
        return command_impl(cmd, std::forward<Args>(args)...);
    }


private:
    std::shared_ptr<Logger> logger;
    redisOptions redis_opt;
    redisContext* redis_ctx;
    
    std::string host, source_addr;
    int port;
    timeval connect_tv, command_tv;
    int alive_interval, max_tries;
    std::chrono::milliseconds retry_interval;

    std::string proc_error(int flag, const std::string& error_pre);

    template<typename... Args>
    Reply command_impl(const char* cmd, Args&&... args){
        redisReply* reply;
        for(int i = 1; i <= max_tries; i++){
            reply = static_cast<redisReply*>(redisCommand(redis_ctx, cmd,
                                             std::forward<Args>(args)...));
            if(reply != NULL){
                return Reply(reply, redisReplyDelete());
            }
            
            proc_error(EXCEPT_PRINT, "command (" +
                       std::string(cmd) + ") error: ");
            if(i == max_tries){
                proc_error(EXCEPT_THROW, "command (" +
                           std::string(cmd) + ") exception: ");
            }

            std::this_thread::sleep_for(retry_interval);
            if(redis_ctx->err == REDIS_ERR_IO || redis_ctx->err == REDIS_ERR_EOF){
                reconnect();
            }
            
            logger->put_warn(LOG_ZONE_REDIS, "command retry: #", i);
        }
    }

};
