#include "redis_base.h"

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

RedisBase::~RedisBase(){
    disconnect();
}

void RedisBase::connect(){
    std::cerr << "connecting..." << std::endl;
    if(redis_ctx != NULL){
        throw RedisError("connecting error: context not null before new connecting");
    }
    
    for(int i = 1; i <= max_retries; i++){
        redis_ctx = redisConnectWithOptions(&redis_opt);
        if(redis_ctx == NULL){
            throw RedisError("fatal: can't allocate context");
        }else if(redis_ctx->err){
            proc_error(i < max_retries ? EXCEPT_PRINT : EXCEPT_BOTH,
                       "connecting error: ");
        }else break;
        
        std::this_thread::sleep_for(retry_interval);
        std::cerr << "connecting retry: #" << i << std::endl;
    }

    if(redisEnableKeepAliveWithInterval(redis_ctx, alive_interval) != REDIS_OK){
        proc_error(EXCEPT_PRINT, "setting socket error: ");
    }
    std::cerr << "connected" << std::endl;
}

void RedisBase::disconnect(){
    redisFree(redis_ctx);
    std::cerr << "disconnected" << std::endl;
}

void RedisBase::reconnect(){
    std::cerr << "reconnecting..."  << std::endl;
    if(redis_ctx == NULL){
        throw RedisError("reconnecting error: context is null before reconnecting");
    }
    
    for(int i = 1; i <= max_retries; i++){
        if(redisReconnect(redis_ctx) != REDIS_OK){
            proc_error(i < max_retries ? EXCEPT_PRINT : EXCEPT_BOTH,
                       "reconnecting error: ");
        }else break;

        std::this_thread::sleep_for(retry_interval);
        std::cerr << "reconnecting retry: #" << i << std::endl;
    }
    std::cerr << "connected" << std::endl;
}

std::string RedisBase::proc_error(int flag, const std::string& error_pre){
    std::string error_msg;
    
    if(redis_ctx->err == REDIS_ERR_IO){
        error_msg = error_pre + "io error(" + std::to_string(errno) + ')';
    }else if(redis_ctx->err){
        error_msg = error_pre + redis_ctx->errstr;
    }else{
        error_msg = error_pre + "unknown error";
    }

    if(flag & 1)std::cerr << error_msg << std::endl;
    if(flag & 2)throw RedisError(error_msg);

    return error_msg;
}


