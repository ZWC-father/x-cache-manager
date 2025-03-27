#include <chrono>
#include <format>
#include <iostream>
#include <iterator>
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

#define EXCEPT_NONE  0
#define EXCEPT_PRINT 1
#define EXCEPT_THROW 2
#define EXCEPT_BOTH  3

#define TYPE_EXCEPT  0  //serious exception
#define TYPE_ERROR   1  //REDIS_REPLY_ERROR
#define TYPE_NIL     2  //REDIS_REPLY_NIL
#define TYPE_STATUS  3  //REDIS_REPLY_STATUS
#define TYPE_INVALID 4  //unexpected return type or too many entries

#define PORT         6379
#define SRC_ADDR     "127.0.0.1"
#define CONN_TIME    1000
#define CMD_TIME     1000
#define ALIVE_INTVL  30000
#define RETRY_INTVL  200
#define MAX_RETRY    3

template<typename T>
struct always_false : std::false_type {};

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
    struct OtherType{
        int type;
        std::string msg;
    };
    RedisBase(const std::string& h, int p, const std::string& src_a, int con_t,
              int cmd_t, int al_int, int rt_int, int max_rt);
    ~RedisBase();
    
    void connect();
    void disconnect();
    void reconnect();

    //no throw guarantee: this func will throw exception only if
    //there is a connection/io issue (when the client can't get reply from server).
    //binary-safe: cmd is not binary-safe
    template<typename T, typename... Args>
    std::variant<OtherType, T> command_single(const char* cmd, const Args&&... args){

        std::unique_ptr<redisReply, redisReplyDelete> reply;
        reply = NULL;
        redis_command_impl(reply, cmd, std::forward<Args>(args)...);
        
        if(reply == NULL){
            return (OtherType){TYPE_EXCEPT,
                   proc_error(EXCEPT_NONE, "command error: ")};
        }

        if(reply->type == REDIS_REPLY_NIL){
            return (OtherType){TYPE_NIL, {}};
        }

        if(reply->type == REDIS_REPLY_ERROR){
            return (OtherType){TYPE_ERROR,
                   std::string(reply->str, reply->len)};
        }

        if(reply->type == REDIS_REPLY_STATUS){
            return (OtherType){TYPE_STATUS,
                   std::string(reply->str, reply->len)};
        }

        if(reply->elements > 1){
            return (OtherType){TYPE_INVALID,
                   "command error: expecting one element"};
        }

        if constexpr(std::is_same_v<T, int>){
            if(reply->type != REDIS_REPLY_INTEGER){
                return (OtherType){TYPE_INVALID,
                       "command error: expecting type int"};
            }
            return static_cast<int>(reply->integer);
        }else if constexpr(std::is_same_v<T, int64_t>){
            if(reply->type != REDIS_REPLY_INTEGER){
                return (OtherType){TYPE_INVALID,
                       "command error: expecting type int64_t"};
            }
            return reply->integer;
        }else if constexpr(std::is_same_v<T, double>){
            if(reply->type != REDIS_REPLY_DOUBLE){
                return (OtherType){TYPE_INVALID,
                       "command error: expecting type double"};
            }
            return reply->dval;
        }else if constexpr(std::is_same_v<T, std::string>){
            if(reply->type != REDIS_REPLY_STRING &&
               reply->type != REDIS_REPLY_BIGNUM){
                return (OtherType){TYPE_INVALID,
                       "command error: expecting type std::string"};
            }
            return std::string(reply->str, reply->len); 
        }else{
            static_assert(always_false<T>::value, "unsupported type in command_single()");
        }
    }


private:
    std::thread connect_daemo;
    redisOptions redis_opt;
    redisContext* redis_ctx;
    
    std::string host, source_addr;
    int port;
    timeval connect_tv, command_tv;
    int alive_interval, max_retries;
    std::chrono::milliseconds retry_interval;

    std::string proc_error(int flag, const std::string& error_pre);

    template<typename... Args>
    void redis_command_impl(redisReply* reply,
                            const char* cmd, const Args&&... args){
        for(int i = 1; i <= max_retries; i++){
            reply = redisCommand(redis_ctx, cmd,
                                 std::forward<Args>(args)...);
            if(reply != NULL)return;
            
            proc_error(EXCEPT_PRINT, "command (" +
                       std::string(cmd) + ") error: "); 
            
            if(redis_ctx->err != REDIS_ERR_IO &&
               redis_ctx->err != REDIS_ERR_EOF &&
               redis_ctx->err != REDIS_ERR_TIMEOUT){
                proc_error(EXCEPT_THROW, "command (" +
                           std::string(cmd) + ") error: ");  
            }

            if(i == max_retries)return;
            
            std::this_thread::sleep_for(retry_interval);
            reconnect();
        }
    }
   
};
