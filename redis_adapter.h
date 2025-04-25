#include "logger.h"
#include "logging_zones.h"
#include "redis_base.h"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <hiredis/hiredis.h>
#include <hiredis/read.h>
#include <memory>
#include <ratio>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>

class RedisAdapter : private RedisBase{
public:
    struct RedisReplyError{std::string str;};
    struct RedisReplyNil{};
    struct RedisReplyStatus{
        std::string str;
        RedisReplyStatus(const std::string& str) : str(str) {}
        bool operator==(const RedisReplyStatus& other){return other.str == str;}
    };
    struct RedisReplyInteger{
        int64_t val;
        RedisReplyInteger(int64_t val) : val(val) {}
        bool operator==(const RedisReplyInteger& other){return other.val == val;}
    };
    struct RedisReplyDouble{
        double val;
        std::string str;
        RedisReplyDouble(double val) : val(val) {}
        RedisReplyDouble(const std::string& str) : str(str) {}
        RedisReplyDouble(double val, const std::string& str) : val(val), str(str) {}
        bool operator==(const RedisReplyDouble& other){return other.val == val || other.str == str;}
    };
    struct RedisReplyString{
        std::string str;
        RedisReplyString(const std::string& str) : str(str) {}
        bool operator==(const RedisReplyString& other){return other.str == str;}
    };

    //struct RedisReplyUnexpected{int type;};

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
    std::variant<T, RedisReplyNil, RedisReplyError> command_single(const char* cmd, Args&&... args){
        Reply reply = command_impl(cmd, std::forward<Args>(args)...);
        //logger->put_debug(LOG_ZONE_REDIS, "command reply type: ", reply->type);
        return reply_proc<T>(reply.get());
    }

    template<typename T, typename... Args>
    std::vector<std::variant<T, RedisReplyNil, RedisReplyError>> command_multi(const char* cmd, Args&&... args){
        Reply reply = command_impl(cmd, std::forward<Args>(args)...);
        //logger->put_debug(LOG_ZONE_REDIS, "command reply type: ", reply->type);
        
        if(reply->type != REDIS_REPLY_ARRAY){ // only support RESP2 and simple array with unique type
            return {reply_proc<T>(reply.get())};
        }
       
        std::vector<std::variant<T, RedisReplyNil, RedisReplyError>> vec(reply->elements);
        for(size_t i = 0; i < reply->elements; i++){
            vec[i] = reply_proc<T>(reply->element[i]);
        }

        return vec;
    }


    template<typename T, typename... Args>
    static const std::string serialize(const T& first, const Args&... rest){
        return serialize_impl(first) + serialize(rest...);
    }

    template<typename T, typename... Args>
    static const bool deserialize(const std::string& str, T& first, Args&... rest){
        size_t offset = 0;
        return deserialize_all(str, offset, first, rest...);
    }
    
private:
    bool upper_retry;
    int upper_retry_interval;
    std::shared_ptr<Logger> logger;

    template<typename... Args>
    Reply command_impl(const char* cmd, Args&&... args){
        Reply reply;
        try{
            reply = command(cmd, std::forward<Args>(args)...);
        }catch(const std::runtime_error& e){
            if(upper_retry)logger->put_error(LOG_ZONE_REDIS, e.what());
            else throw;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(upper_retry_interval));
           
            logger->put_warn(LOG_ZONE_REDIS, "command retry by adapter");
            if(connected){
                reply = command(cmd, std::forward<Args>(args)...);
            }else{
                reset();
                reply = command(cmd, std::forward<Args>(args)...);
            }
        }
        return reply;
    }

    template<typename T>
    std::variant<T, RedisReplyNil, RedisReplyError> reply_proc(const redisReply* reply){
        if(reply->type == REDIS_REPLY_NIL){
            return (RedisReplyNil){};
        }
        
        if(reply->type == REDIS_REPLY_ERROR){
                return (RedisReplyError){std::string(reply->str, reply->len)};
        }

        if constexpr(std::is_same_v<T, RedisReplyStatus>){
            if(reply->type == REDIS_REPLY_STATUS){
                return (RedisReplyStatus){std::string(reply->str, reply->len)};
            }
        }else if constexpr(std::is_same_v<T, RedisReplyInteger>){
            if(reply->type == REDIS_REPLY_INTEGER){
                return (RedisReplyInteger){reply->integer};
            }
        }else if constexpr(std::is_same_v<T, RedisReplyDouble>){
            if(reply->type == REDIS_REPLY_DOUBLE){
                return (RedisReplyDouble){reply->dval, std::string(reply->str, reply->len)};
            }
        }else if constexpr(std::is_same_v<T, RedisReplyString>){
            if(reply->type == REDIS_REPLY_STRING){
                return (RedisReplyString){std::string(reply->str, reply->len)};
            }
        }else{
            static_assert(true, "invaild type in RedisAdapter::reply_proc()");
        }

        throw RedisError("command unexpected return type: " + std::to_string(reply->type)); 
    }

    static const std::string serialize(){return "";}

    template<typename T>
    static const typename std::enable_if<std::is_same<T, int>::value, std::string>::type
    serialize_impl(const T& val){
        uint32_t u = static_cast<uint32_t>(val) ^ 0x80000000U;
        char buf[4];
        buf[0] = (u >> 24) & 0xFF;
        buf[1] = (u >> 16) & 0xFF;
        buf[2] = (u >> 8) & 0xFF;
        buf[3] = u & 0xFF;
        return std::string(buf, 4);
    }

    template<typename T>
    static const typename std::enable_if<std::is_same<T, int64_t>::value, std::string>::type
    serialize_impl(const T &val){
        uint64_t u = static_cast<uint64_t>(val) ^ 0x8000000000000000ULL;
        char buf[8];
        for (int i = 0; i < 8; ++i){
            buf[i] = (u >> (56 - 8*i)) & 0xFF;
        }
        return std::string(buf, 8);
    }
    
    template<typename T>
    static const typename std::enable_if<std::is_same<T, uint64_t>::value || std::is_same<T, size_t>::value, std::string>::type
    serialize_impl(const T &val){
        char buf[8];
        for (int i = 0; i < 8; ++i){
            buf[i] = (val >> (56 - 8*i)) & 0xFF;
        }
        return std::string(buf, 8);
    }

    static const std::string serialize_impl(const std::string& str){ // put it to last, as it is not sorted.
        uint32_t len = str.size();
        char buf[4];
        buf[0] = (len >> 24) & 0xFF;
        buf[1] = (len >> 16) & 0xFF;
        buf[2] = (len >> 8) & 0xFF;
        buf[3] = len & 0xFF;
        return std::string(buf, 4) + str;
    }
    
    static const bool deserialize_all(const std::string& str, size_t& offset){
        return offset == str.size();
    }
    
    template<typename T, typename... Args>
    static const bool deserialize_all(const std::string& str, size_t& offset, T& first, Args&... rest){
        if(!deserialize_impl(str, offset, first))return false;
        return deserialize_all(str, offset, rest...);
    }

    template<typename T>
    static const typename std::enable_if<std::is_same<T, int>::value, bool>::type
    deserialize_impl(const std::string& str, size_t& offset, T& val){
        constexpr size_t bytes = 4;
        if(offset + bytes > str.size())return false;

        uint32_t u = 0;
        for(size_t i = 0; i < bytes; i++){
            u = (u << 8) | static_cast<unsigned char>(str[offset + i]);
        }
        
        offset += bytes;
        u ^= 0x80000000U;
        val = static_cast<int>(u);
        return true;
    }

    template<typename T>
    static const typename std::enable_if<std::is_same<T, int64_t>::value, bool>::type
    deserialize_impl(const std::string& str, size_t& offset, T& val) {
        constexpr size_t bytes = 8;
        if(offset + bytes > str.size())return false;
        
        uint64_t u = 0;
        for(size_t i = 0; i < bytes; i++){
            u = (u << 8) | static_cast<unsigned char>(str[offset + i]);
        }
        
        offset += bytes;
        u ^= 0x8000000000000000ULL;
        val = static_cast<int64_t>(u);
        return true;
    }

    template<typename T>
    static const typename std::enable_if<std::is_same<T, uint64_t>::value || std::is_same<T, size_t>::value, bool>::type
    deserialize_impl(const std::string& str, size_t& offset, T& val){
        constexpr size_t bytes = 8;
        if(offset + bytes > str.size())return false;

        uint64_t u = 0;
        for(size_t i = 0; i < bytes; ++i){
            u = (u << 8) | static_cast<unsigned char>(str[offset + i]);
        }

        offset += bytes;
        val = u;
        return true;
    }


    static const bool deserialize_impl(const std::string& str, size_t& offset, std::string& val){
        constexpr size_t prefix = 4;
        if(offset + prefix > str.size())return false;
        
        uint32_t len = 0;
        for(size_t i = 0; i < prefix; i++){
            len = (len << 8) | static_cast<unsigned char>(str[offset + i]);
        }
    
        offset += prefix;
        if(offset + len > str.size())return false;
        
        val = str.substr(offset, len);
        offset += len;
        return true;
    }

};
