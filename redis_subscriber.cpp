#include "redis_subscriber.h"
#include "logging_zones.h"
#include <cstddef>
#include <event2/event.h>
#include <hiredis/async.h>
#include <hiredis/read.h>

RedisSub::RedisSub(SubManager* manager, redisAsyncContext** ctx,
const std::shared_ptr<Logger>& l, const std::string& h,
int p, const std::string& src_a, int con_t) : redis_ctx(ctx),
logger(l), host(h), port(p), source_addr(src_a),
redis_opt({0}), base(NULL){
    redis_opt.options |= REDIS_OPT_PREFER_IP_UNSPEC;
    REDIS_OPTIONS_SET_TCP(&redis_opt, host.c_str(), port);
    redis_opt.endpoint.tcp.source_addr = source_addr.c_str();
    connect_tv = {con_t / 1000,
                  con_t % 1000 * 1000};
    redis_opt.connect_timeout = &connect_tv;

    base = event_base_new();
    if(base == NULL)throw RedisError("event_base creation failed");
    *redis_ctx = NULL;


}

RedisSub::~RedisSub(){
    free();
    if(base != NULL)event_base_free(base);
}


bool RedisSub::connect(){
    if(*redis_ctx != NULL){
        logger->put_warn(LOG_ZONE_REDIS, "connected before new connection");
        free();
    }
    
    logger->put_info(LOG_ZONE_REDIS, "connecting...");

    *redis_ctx = redisAsyncConnectWithOptions(&redis_opt);
    
    if(*redis_ctx == NULL){
        logger->put_error(LOG_ZONE_REDIS, "connection error: failed to create async context");
        return 0;
    }

    if((*redis_ctx)->err){
        logger->put_error(LOG_ZONE_REDIS, "connection error: ", (*redis_ctx)->errstr);
        redisAsyncFree(*redis_ctx);
        *redis_ctx = NULL;
        return 0;
    }

    *redis_ctx->data = (){redis_ctx, manager};
    
    redisAsyncSetConnectCallback(*redis_ctx, &RedisSub::connect_cb);
    redisAsyncSetDisconnectCallback(*redis_ctx, &RedisSub::disconnect_cb);
    
    if(redisLibeventAttach(*redis_ctx, base) != REDIS_OK){
        logger->put_error(LOG_ZONE_REDIS, "event_base attach error: ", (*redis_ctx)->errstr);  
        redisAsyncFree(*redis_ctx);
        *redis_ctx = NULL;
        return 0;
    }

    return 1;

}

void RedisSub::disconnect(){
    if(*redis_ctx == NULL)return;
    redisAsyncDisconnect(*redis_ctx);
    *redis_ctx = NULL;
}

void RedisSub::free(){
    if(*redis_ctx == NULL)return;
    redisAsyncFree(*redis_ctx);
    *redis_ctx = NULL;
}

void RedisSub::set_null(){
    *redis_ctx = NULL;
}

void RedisSub::connect_cb(const redisAsyncContext* redis, int res){
    auto manager = static_cast<SubManager*>(redis->data);
    std::string msg;
    if(res == REDIS_OK){
        msg = "connected";
    }else{
        if(res != REDIS_ERR_IO){
            msg = "connection error: io error (" + std::to_string(errno) + ')';
        }else{
            msg = "connection error: " + std::string(redis->errstr);
        }
    }
    
    manager->connect_callback(res, msg);
}

void RedisSub::disconnect_cb(const redisAsyncContext* redis, int res){
    auto manager = static_cast<SubManager*>(redis->data);
    std::string msg;
    if(res == REDIS_OK){
        msg = "disconnected";
    }else{
        if(res != REDIS_ERR_IO){
            msg = "disconnection error: io error (" + std::to_string(errno) + ')';
        }else{
            msg =  "disconnection error: " +  std::string(redis->errstr);
        }
    }
    
    manager->disconnect_callback(res, msg);
}


SubManager::SubManager(const std::shared_ptr<Logger>& logger, const std::string& host,
int port, const std::string& source_addr, int connect_timeout, int retry_interval) :
logger(logger), host(host), port(port), source_addr(source_addr),
connect_timeout(connect_timeout), retry_interval(retry_interval) {}

void init(){
    
}

