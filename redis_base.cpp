#include "redis_base.h"
#include <cstdio>
#include <hiredis/hiredis.h>
#include <hiredis/read.h>
#include <string>
#include <system_error>

RedisBase::RedisBase(const std::string& h, int p = PORT, 
const std::string& src_a = SRC_ADDR, int con_t = CONN_TIME,
int cmd_t = CMD_TIME, int al_int = ALIVE_INTVL, int rt_int = RETRY_INTVL,
int max_rt = MAX_RETRY) : host(h), port(p), source_addr(src_a),
alive_interval(al_int), retry_interval(rt_int), max_retries(max_rt),
redis_opt({0}), redis_ctx(NULL){
    
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

void RedisBase::connect(){
    reset_count();
    if(redis_ctx != NULL){
        throw RedisError("connecting error: not a null context before connecting");
    }
    
    do_redis([this](){
        redis_ctx = redisConnectWithOptions(&redis_opt);
        if(redis_ctx == NULL){
            throw RedisError("fatal: can't allocate redis context");
        }
        if(redis_ctx->err)perror("connecting error: ");
    });

    if(redisEnableKeepAliveWithInterval(redis_ctx, alive_interval) != REDIS_OK){
        perror("setting socket error: ");
    }
}

void RedisBase::disconnect(){
    reset_count();
    redisFree(redis_ctx);
}

void RedisBase::reconnect(){
    if(redisReconnect(redis_ctx) != REDIS_OK){
        perror("reconnecting error: ");
    }
}

void RedisBase::perror(const std::string& error_msg){
    error_count.add(redis_ctx->err);
    if(redis_ctx->err == REDIS_ERR_IO){
        throw RedisError(error_msg + "io error(" +
                         std::to_string(errno) + ')');
    }else if(redis_ctx->err){
        throw RedisError(error_msg + redis_ctx->errstr);
    }else{
        throw RedisError(error_msg + "unknown error");
    }
}

void RedisBase::reset_count(){
    error_count = {0, 0, 0, 0};
}
