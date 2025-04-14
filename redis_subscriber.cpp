#include "redis_subscriber.h"
#include "logging_zones.h"
#include <hiredis/async.h>

RedisBase::RedisBase(const std::shared_ptr<Logger>& l,
const std::string& h, int p, const std::string& src_a,
int con_t, int al_int) :
logger(l), host(h), port(p), source_addr(src_a), alive_interval(al_int),
redis_opt({0}), redis_ctx(NULL), connected(false){
    
    redis_opt.options |= REDIS_OPT_PREFER_IP_UNSPEC;
    REDIS_OPTIONS_SET_TCP(&redis_opt, host.c_str(), port);
    redis_opt.endpoint.tcp.source_addr = source_addr.c_str();
    connect_tv = {con_t / 1000,
                  con_t % 1000 * 1000};
    redis_opt.connect_timeout = &connect_tv;
    
}

RedisBase::~RedisBase(){
}

void RedisBase::connect(){
    if(redis_ctx != NULL){
        logger->put_error(LOG_ZONE_REDIS, "context not null before connecting");
        connect_callback(CONTEXT_INIT_FAIL);
        return;
    }

    logger->put_info(LOG_ZONE_REDIS, "connecting");

    if(base == NULL)base = event_base_new();
    if(base == NULL){
        logger->put_error(LOG_ZONE_REDIS, "failed to create event_base");
        connect_callback(CONTEXT_INIT_FAIL);
        return;
    }

    redis_ctx = redisAsyncConnectWithOptions(&redis_opt);
    
    if(redis_ctx == NULL){
        logger->put_error(LOG_ZONE_REDIS, "failed to create redis context");
        connect_callback(CONTEXT_INIT_FAIL);
        return;
    }
    
    if(redis_ctx->err){
        logger->put_error(LOG_ZONE_REDIS, "redis context init error: ", redis_ctx->errstr);
        redisAsyncFree(redis_ctx);
        redis_ctx = NULL;
        connect_callback(CONTEXT_INIT_FAIL);
        return;
    }
    
    if(redisLibeventAttach(redis_ctx, base) != REDIS_OK){
        logger->put_error(LOG_ZONE_REDIS, "failed to attach context to event_base: ", redis_ctx->errstr);  
        redisAsyncFree(redis_ctx);
        redis_ctx = NULL;
        connect_callback(CONTEXT_INIT_FAIL);
        return;
    }

}

void RedisBase::disconnect(){
    if(redis_ctx == NULL)return;
    redisFree(redis_ctx);
    redis_ctx = NULL;
    logger->put_info(LOG_ZONE_REDIS, "disconnected");
    connected = false;
}

void RedisBase::reconnect(){
}

std::string RedisBase::proc_error(int flag, const std::string& error_pre){
    std::string error_msg;
    
    if(redis_ctx->err == REDIS_ERR_IO){
        error_msg = error_pre + "io error (" + std::to_string(errno) + ')';
    }else if(redis_ctx->err){
        error_msg = error_pre + redis_ctx->errstr;
    }else{
        error_msg = error_pre + "unknown";
    }

    if(flag & 1)logger->put_error(LOG_ZONE_REDIS, error_msg);
    if(flag & 2)throw RedisError(error_msg);

    return error_msg;
}

