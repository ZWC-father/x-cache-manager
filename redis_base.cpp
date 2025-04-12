#include "redis_base.h"
#include "logging_zones.h"

RedisBase::RedisBase(const std::shared_ptr<Logger>& l,
const std::string& h, int p,  const std::string& src_a,
int con_t, int cmd_t, int al_int, int rt_int, int max_rt) :
logger(l), host(h), port(p), source_addr(src_a), alive_interval(al_int),
retry_interval(rt_int), max_tries(max_rt + 1), redis_opt({0}),
redis_ctx(NULL), connected(false){
    
    redis_opt.options |= REDIS_OPT_PREFER_IP_UNSPEC;
    REDIS_OPTIONS_SET_TCP(&redis_opt, host.c_str(), port);
    redis_opt.endpoint.tcp.source_addr = source_addr.c_str();
    connect_tv = {con_t / 1000,
                  con_t % 1000 * 1000};
    command_tv = {cmd_t / 1000,
                  cmd_t % 1000 * 1000};
    redis_opt.connect_timeout = &connect_tv;
    redis_opt.command_timeout = &command_tv; 

}

RedisBase::~RedisBase(){
    disconnect();
}

void RedisBase::connect(){
    logger->put_info(LOG_ZONE_REDIS, "connecting");
    
    if(redis_ctx != NULL){
        throw RedisError("connecting exception: context not null before new connecting");
    }
    
    for(int i = 1; i <= max_tries; i++){
        redis_ctx = redisConnectWithOptions(&redis_opt);
        if(redis_ctx == NULL){
            throw RedisError("connecting exception: can't allocate context");
        }else if(redis_ctx->err){
            if(i < max_tries)proc_error(EXCEPT_PRINT, "connecting error: ");
            else proc_error(EXCEPT_THROW, "connecting exception: ");
        }else break;
        
        std::this_thread::sleep_for(retry_interval);
        logger->put_warn(LOG_ZONE_REDIS, "connecting retry: #", i);
    }

    if(redisEnableKeepAliveWithInterval(redis_ctx, alive_interval) != REDIS_OK){
        proc_error(EXCEPT_PRINT, "setting socket error: ");
    }

    logger->put_info(LOG_ZONE_REDIS, "connected");
    connected = true;
}

void RedisBase::disconnect(){
    if(redis_ctx == NULL)return;
    redisFree(redis_ctx);
    redis_ctx = NULL;
    logger->put_info(LOG_ZONE_REDIS, "disconnected");
    connected = false;
}

void RedisBase::reconnect(){
    logger->put_info(LOG_ZONE_REDIS, "reconnecting");
    connected = false;
    
    if(redis_ctx == NULL){
        throw RedisError("reconnecting exception: context is null before reconnecting");
    }
    
    for(int i = 1; i <= max_tries; i++){
        if(redisReconnect(redis_ctx) != REDIS_OK){
            if(i < max_tries)proc_error(EXCEPT_PRINT, "reconnecting error: ");
            else proc_error(EXCEPT_THROW, "reconnecting exception: ");
        }else break;

        std::this_thread::sleep_for(retry_interval);
        logger->put_warn(LOG_ZONE_REDIS, "reconnecting retry: #", i);
    }

    logger->put_info(LOG_ZONE_REDIS, "connected");
    connected = true;
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

