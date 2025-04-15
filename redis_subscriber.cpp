#include "redis_subscriber.h"
#include "logging_zones.h"
#include <chrono>
#include <cstddef>
#include <event2/event.h>
#include <hiredis/async.h>
#include <hiredis/read.h>
#include <mutex>
#include <thread>

RedisSub::RedisSub(const std::shared_ptr<Logger>& l,
ConnectCallBack con_cb, DisconnectCallBack dcon_cb,
const std::string& h, int p, const std::string& src_a,
int con_t, int al_int) :
host(h), port(p), source_addr(src_a), alive_interval(al_int),
redis_opt({0}), redis_ctx(NULL){
    if(sub_aux != nullptr)throw RedisError("duplicated subscriber");
    sub_aux = new SubAUX(l); 

    redis_opt.options |= REDIS_OPT_PREFER_IP_UNSPEC;
    REDIS_OPTIONS_SET_TCP(&redis_opt, host.c_str(), port);
    redis_opt.endpoint.tcp.source_addr = source_addr.c_str();
    connect_tv = {con_t / 1000,
                  con_t % 1000 * 1000};
    redis_opt.connect_timeout = &connect_tv;

    base = event_base_new();
    if(base == NULL)throw RedisError("event_base creation failed");
}

RedisSub::~RedisSub(){
    blocking_disconnect();
    if(base != NULL)event_base_free(base);
    delete sub_aux;
}

void RedisSub::blocking_disconnect(){
    while(sub_aux->connected == -1){
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    
    }
    if(sub_aux->connected == 1){
        redisAsyncFree(redis_ctx);
        std::unique_lock<std::mutex> lock(sub_aux->event_lock);
        if(sub_aux->connected){
            sub_aux->cv.wait(lock, [this]{return !sub_aux->connected;});
        }
    }
}

void RedisSub::connect(){
    if(sub_aux->connected == -1)throw RedisError("callback pending: aborted");
    if(sub_aux->connected == 1){
        logger->put_warn(LOG_ZONE_REDIS, "connected before new connection");
        blocking_disconnect();
    }
    
    sub_aux->connected = -1;
    logger->put_info(LOG_ZONE_REDIS, "connecting...");

    redis_ctx = redisAsyncConnectWithOptions(&redis_opt);
    
    if(redis_ctx == NULL){
        logger->put_error(LOG_ZONE_REDIS, "connection error: failed to create async context");
        sub_aux->connected = 0;
        connect_callback(CONTEXT_INIT_FAIL);
        return;
    }
    
    if(redis_ctx->err){
        logger->put_error(LOG_ZONE_REDIS, "connection error: ", redis_ctx->errstr);
        redisAsyncFree(redis_ctx);
        sub_aux->connected = 0;
        connect_callback(CONTEXT_INIT_FAIL);
        return;
    }

    
    redisAsyncSetConnectCallback(redis_ctx, &RedisSub::connect_cb);
    redisAsyncSetDisconnectCallback(redis_ctx, &RedisSub::disconnect_cb);
    
    if(redisLibeventAttach(redis_ctx, base) != REDIS_OK){
        logger->put_error(LOG_ZONE_REDIS, "event_base attach error: ", redis_ctx->errstr);  
        redisAsyncFree(redis_ctx);
        sub_aux->connected = 0;
        connect_callback(CONTEXT_INIT_FAIL);
    }

}

void RedisSub::disconnect(){
    if(sub_aux->connected == -1)throw RedisError("callback pending: aborted");
    if(sub_aux->connected == 0){
        logger->put_warn(LOG_ZONE_REDIS, "already disconnected");
        return;
    }

    redisAsyncDisconnect(redis_ctx);
}

void RedisSub::connect_cb(const redisAsyncContext* redis, int res){
    if(sub_aux == nullptr)return;
    if(res != REDIS_OK)sub_aux->connected = 1;
    else{
        sub_aux->connected = 0;
        if(res != REDIS_ERR_IO){
            sub_aux->logger->put_error(LOG_ZONE_REDIS, "connection error: io error (" 
                                       + std::to_string(errno) + ')');
        }else{
            sub_aux->logger->put_error(LOG_ZONE_REDIS, "connection error: "
                                       + std::string(redis->errstr));
    
        }
    }
    connect_callback(res);
}

void RedisSub::disconnect_cb(const redisAsyncContext* redis, int res){
    if(res == REDIS_OK){
        sub_aux->logger->put_info(LOG_ZONE_REDIS, "disconnected");
    }else{
        if(res != REDIS_ERR_IO){
            sub_aux->logger->put_error(LOG_ZONE_REDIS, "disconnection error: io error (" + 
                              std::to_string(errno) + ')');
        }else{
            sub_aux->logger->put_error(LOG_ZONE_REDIS, "disconnection error: " + 
                              std::string(redis->errstr));
        }
    }
    
    sub_aux->connected = 0;
    sub_aux->cv.notify_all();
    disconnect_callback(res);
}

