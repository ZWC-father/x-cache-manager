#include "redis_subscriber.h"
#include "logging_zones.h"
#include <algorithm>
#include <cstdio>
#include <event2/event.h>
#include <exception>
#include <hiredis/async.h>
#include <hiredis/read.h>
#include <memory>
#include <stdexcept>
#include <thread>

RedisSub::RedisSub(SubManager* manager,
const std::shared_ptr<Logger>& l, const std::string& h,
int p, const std::string& src_a, int con_t) : manager(manager),
logger(l), host(h), port(p), source_addr(src_a),
redis_opt({0}){
    redis_opt.options |= REDIS_OPT_PREFER_IP_UNSPEC;
    REDIS_OPTIONS_SET_TCP(&redis_opt, host.c_str(), port);
    redis_opt.endpoint.tcp.source_addr = source_addr.c_str();
    connect_tv = {con_t / 1000,
                  con_t % 1000 * 1000};
    redis_opt.connect_timeout = &connect_tv;

    base = event_base_new();
    if(base == NULL)throw RedisError("event_base creation failed");
    event_guard = evtimer_new(base, &RedisSub::dummy_cb, NULL);
    if(event_guard == NULL)throw RedisError("event guard creation failed");
    event_add(event_guard, &connect_tv);

    manager->redis_ctx = NULL;
}

RedisSub::~RedisSub(){
    free();
    if(event_guard != NULL)event_free(event_guard);
    if(base != NULL)event_base_free(base);
}


int RedisSub::connect(){
    if(manager->redis_ctx != NULL){
        throw RedisError("context not null before new connection");
    }
    
    logger->put_info(LOG_ZONE_REDIS, "connecting...");

    manager->redis_ctx = redisAsyncConnectWithOptions(&redis_opt);
    
    if(manager->redis_ctx == NULL){
        logger->put_error(LOG_ZONE_REDIS, "connection error: failed to create async context");
        return -1;
    }

    if(manager->redis_ctx->err){
        logger->put_error(LOG_ZONE_REDIS, "connection error: ",
                          manager->redis_ctx->errstr);
        int err = manager->redis_ctx->err;
        free();
        return err;
    }

    manager->redis_ctx->data = manager;
    
    redisAsyncSetConnectCallback(manager->redis_ctx, &RedisSub::connect_cb);
    redisAsyncSetDisconnectCallback(manager->redis_ctx, &RedisSub::disconnect_cb);
    
    if(redisLibeventAttach(manager->redis_ctx, base) != REDIS_OK){
        logger->put_error(LOG_ZONE_REDIS, "event_base attach error: ",
                          manager->redis_ctx->errstr);  
        int err = manager->redis_ctx->err;
        free();
        return err;
    }

    return 0;

}

void RedisSub::disconnect(){
    if(manager->redis_ctx == NULL)return;
    logger->put_info(LOG_ZONE_REDIS, "disconnecting...");
    redisAsyncDisconnect(manager->redis_ctx);
    manager->redis_ctx = NULL;
}

void RedisSub::free(){
    if(manager->redis_ctx == NULL)return;
    logger->put_info(LOG_ZONE_REDIS, "freeing...");
    redisAsyncFree(manager->redis_ctx);
    manager->redis_ctx = NULL;
}

void RedisSub::run(){
    event_base_dispatch(base);
}

int RedisSub::subscribe(const std::string& channel, RedisCallback callback,
                        bool psub){
    if(manager->redis_ctx == NULL)throw RedisError("redis context is null");
    
    const char* cmd = psub ? "PSUBSCRIBE %s" : "SUBSCRIBE %s";
    int res = redisAsyncCommand(manager->redis_ctx, callback,
                                manager, cmd);
    return res;
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
        manager->redis_ctx = NULL;
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

    manager->redis_ctx = NULL;
    manager->disconnect_callback(res, msg);
}


SubManager::SubManager(const std::shared_ptr<Logger>& logger, const std::string& host,
int port, const std::string& source_addr, int connect_timeout, int max_retries,
int retry_interval) : logger(logger), host(host), port(port), source_addr(source_addr),
connect_timeout(connect_timeout), max_tries(max_retries + 1),
retry_interval(retry_interval), redis_ctx(NULL), try_count(0){
    redis = new RedisSub(this, logger, host,
                         port, source_addr, connect_timeout);
    fut = prom.get_future();
    except_fut = except_prom.get_future();
}

void SubManager::init(){
    event_loop = std::thread([this]{redis->run();});
    connection_loop = std::thread(&SubManager::connection_manager, this);
}

void SubManager::uninit(){

}

void SubManager::wait(){
    
}

void SubManager::do_connect(){
    if(!redis->connect()){
        pending = false;
        throw RedisError("connection init exception");
    }
    try_count++;
    
}

void SubManager::connection_manager(){
    while(1){
        bool status = fut.get();
        try{
            if(status){
                if(try_count == max_tries)throw RedisError("too many connection failures");
                std::this_thread::sleep_for(retry_interval);
                logger->put_info(LOG_ZONE_REDIS, "connecting retry #", try_count);
                if(pending = )do_connect();
            }else break;
        }catch(...){
            except_prom.set_exception(std::current_exception());
        }
    }
}

void SubManager::connect_callback(int res, const std::string& msg){
    if(stop_signal){
        logger->put_warn(LOG_ZONE_REDIS, "callback aborted, quiting...");
        prom.set_value(false);
        return;
    }
    
    if(res == REDIS_OK)logger->put_info(LOG_ZONE_REDIS, msg);
    else{
        logger->put_error(LOG_ZONE_REDIS, msg);
        prom.set_value(true);
    }
    
}


void SubManager::disconnect_callback(int res, const std::string& msg){
    if(stop_signal){
        logger->put_warn(LOG_ZONE_REDIS, "callback aborted, quiting...");
        prom.set_value(false);
        return;
    }
    
    if(res == REDIS_OK)logger->put_info(LOG_ZONE_REDIS, msg);
    else{
        logger->put_error(LOG_ZONE_REDIS, msg);
        prom.set_value(true);
    }
    
}
