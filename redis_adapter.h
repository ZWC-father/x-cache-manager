#include "logger.h"
#include "logging_zones.h"
#include "redis_base.h"
#include <chrono>
#include <hiredis/hiredis.h>
#include <hiredis/read.h>
#include <memory>
#include <ratio>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <variant>

class RedisAdapter : private RedisBase{
public:
    struct RedisReplyInteger{int64_t val;};
    struct RedisReplyDouble{double val; std::string str;};
    struct RedisReplyStatus{std::string str;};
    struct RedisReplyString{std::string str;};
    struct RedisReplyStringArray{
        std::vector<std::string> vec;
        RedisReplyStringArray(size_t size){vec.resize(size);}
    };
    
    struct RedisReplyError{std::string str;};
    struct RedisReplyUnexpected{int type;};


    RedisAdapter(const std::shared_ptr<Logger>& logger, const std::string& host,
                 int port, const std::string& src_addr, int connect_timeout,
                 int command_timeout, int alive_interval, int retry_interval, 
                 int max_retries, bool enable_upper_retry) :
                 RedisBase(logger, host, port, src_addr,
                 connect_timeout, command_timeout, alive_interval,
                 retry_interval, max_retries), upper_retry(enable_upper_retry),
                 upper_retry_interval(retry_interval), logger(logger) {}
    
    void init(){
        connect();
    }

    void reset(){
        logger->put_warn(LOG_ZONE_REDIS, "redis context reset");
        disconnect(); //free redis context and reconnect completely
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        connect();
    }


    template<typename T, typename... Args>
    std::variant<T, RedisReplyError, RedisReplyUnexpected> command(const char* cmd, Args&&... args){
        Reply reply;
        try{
            reply = command_single(cmd, std::forward<Args>(args)...);
        }catch(const std::runtime_error& e){
            if(upper_retry)logger->put_error(LOG_ZONE_REDIS, e.what());
            else throw;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(upper_retry_interval));
           
            logger->put_warn(LOG_ZONE_REDIS, "command retry by adapter");
            if(connected){
                reply = command_single(cmd, std::forward<Args>(args)...);
            }else{
                reset();
                reply = command_single(cmd, std::forward<Args>(args)...);
            }
        }

        if(reply->type == REDIS_REPLY_ERROR){
            return (RedisReplyError){std::string(reply->str, reply->len)};
        }

        if constexpr(std::is_same_v<T, RedisReplyInteger>){
            if(reply->type != REDIS_REPLY_INTEGER){
                return (RedisReplyInteger){reply->integer};
            }
        }else if constexpr(std::is_same_v<T, RedisReplyDouble>){
            if(reply->type != REDIS_REPLY_DOUBLE){
                return (RedisReplyDouble){reply->dval, std::string(reply->str, reply->len)};
            }
        }else if constexpr(std::is_same_v<T, RedisReplyStatus>){
            if(reply->type != REDIS_REPLY_STATUS){
                return (RedisReplyStatus){std::string(reply->str, reply->len)};
            }
        }else if constexpr(std::is_same_v<T, RedisReplyString>){
            if(reply->type != REDIS_REPLY_STRING){
                return (RedisReplyString){std::string(reply->str, reply->len)};
            }
        }else if constexpr(std::is_same_v<T, RedisReplyStringArray>){
            RedisReplyStringArray res(reply->elements);
            bool flag = true;
            for(int i = 0; i < reply->elements; i++){
                if(reply->element[i]->type != REDIS_REPLY_STRING){
                    flag = false;
                    break;
                }
                res.vec[i] = std::string(reply->element[i]->str, reply->element[i]->len);
            }
            if(flag)return res;
        }else{
            static_assert(true, "invaild type in RedisAdapter::command()");
        }

        return (RedisReplyUnexpected){reply->type};
    }
    
private:
    bool upper_retry;
    int upper_retry_interval;
    std::shared_ptr<Logger> logger;
    
};
