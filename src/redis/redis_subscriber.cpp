#include "redis_subscriber.h"
#include "logging_zones.h"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <event2/event.h>
#include <hiredis/async.h>
#include <hiredis/read.h>
#include <memory>
#include <mutex>
#include <vector>

RedisSub::RedisSub(SubManager* manager,
const std::shared_ptr<Logger>& l, const std::string& h,
int p, const std::string& src_a, int con_t) : manager(manager),
logger(l), host(h), port(p), source_addr(src_a),
redis_opt({0}), base(NULL){
    manager->redis_ctx = NULL;
    redis_opt.options |= REDIS_OPT_PREFER_IP_UNSPEC;
    REDIS_OPTIONS_SET_TCP(&redis_opt, host.c_str(), port);
    redis_opt.endpoint.tcp.source_addr = source_addr.c_str();
    connect_tv = {con_t / 1000,
                  con_t % 1000 * 1000};
    redis_opt.connect_timeout = &connect_tv;

    init();
}

RedisSub::~RedisSub(){
    uninit();
}//disconnect must be called manually for a successful connection

void RedisSub::init(){
    if(base == NULL)base = event_base_new();
    if(base == NULL)throw RedisError("event_base creation failed");
}

void RedisSub::uninit(){
    manager->redis_ctx = NULL;
    if(base != NULL)event_base_free(base);
}

int RedisSub::connect(){
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

    
    if(redisLibeventAttach(manager->redis_ctx, base) != REDIS_OK){
        logger->put_error(LOG_ZONE_REDIS, "event_base attach error: ",
                          manager->redis_ctx->errstr);  
        int err = manager->redis_ctx->err;
        free();
        return err;
    }
    
    manager->redis_ctx->data = manager;
    redisAsyncSetConnectCallback(manager->redis_ctx, &RedisSub::connect_cb);
    redisAsyncSetDisconnectCallback(manager->redis_ctx, &RedisSub::disconnect_cb);

    return 0;

}

void RedisSub::disconnect(){
    if(manager->redis_ctx == NULL)return;
    logger->put_info(LOG_ZONE_REDIS, "disconnecting...");
    redisAsyncDisconnect(manager->redis_ctx);
}

void RedisSub::free(){
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
                                manager, cmd, channel.c_str());
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


SubManager::SubManager(SubCallback sub_callback, const std::shared_ptr<Logger>& logger,
const std::string& host, int port, const std::string& source_addr,
int connect_timeout) : sub_callback(sub_callback), logger(logger), host(host),
port(port), source_addr(source_addr), connect_timeout(connect_timeout),
redis_ctx(NULL), redis(nullptr){}

void SubManager::init(){
    status = 0;
    running = false;
    redis = new RedisSub(this, logger, host,
                         port, source_addr, connect_timeout);
}

void SubManager::uninit(){
    std::unique_lock<std::mutex> lock(connect_lock);
    if(redis == nullptr){
        logger->put_error(LOG_ZONE_REDIS, "redis not initialized");
        return;
    }
    
    if(running){
        redis->disconnect();
        logger->put_info(LOG_ZONE_REDIS, "event loop exit");
        cv.wait(lock);
        free();
    }else{
        logger->put_warn(LOG_ZONE_REDIS, "event loop not running");
        free();
    }
}

void SubManager::free(){
    delete redis;
    redis = nullptr;
    status = 0;
}

void SubManager::connect(){
    std::unique_lock<std::mutex> lock(connect_lock);
    if(redis == nullptr)return;
    
    running = true;
    if(int res = redis->connect()){
        running = false;
        throw RedisError("connection init exception: " + std::to_string(res));
    }
    
    lock.unlock();
    redis->run();
    lock.lock();

    if(status == -1){
        running = false;
        cv.notify_one();
        throw RedisError("connection exception");
    }
    running = false;
    cv.notify_one();
}

int SubManager::subscribe(){
    std::unique_lock<std::mutex> lock(connect_lock);
    if(redis == nullptr){
        logger->put_error(LOG_ZONE_REDIS, "redis not initialized");
        return -1;
    }
    
    return redis->subscribe(CHANNEL, command_callback, PSUB);
    
    
}

void SubManager::connect_callback(int res, const std::string& msg){
    if(res == REDIS_OK){
        logger->put_info(LOG_ZONE_REDIS, msg);
        status = 1;
    }else{
        logger->put_error(LOG_ZONE_REDIS, msg);
        status = -1;
    }
}


void SubManager::disconnect_callback(int res, const std::string& msg){
    if(res == REDIS_OK){
        logger->put_info(LOG_ZONE_REDIS, msg);
        status = 0;
    }else{
        logger->put_error(LOG_ZONE_REDIS, msg);
        status = -1;
    }
}

void SubManager::command_callback(redisAsyncContext *ctx,
                                  void *r, void *privdata){
    if(r == NULL)return;
    auto reply = static_cast<redisReply*>(r);
    auto manager = static_cast<SubManager*>(privdata);
    if(reply->type == REDIS_REPLY_ERROR){
        manager->sub_callback((RedisReplyError)
                              {std::string(reply->str, reply->len)});
  
    }else if(reply->type == REDIS_REPLY_STATUS){
        manager->sub_callback((RedisReplyStatus)
                              {std::string(reply->str, reply->len)});

    }else if(reply->type == REDIS_REPLY_STRING){
        manager->sub_callback((RedisReplyStringArray)
                {{std::string(reply->str, reply->len)}});
    
    }else if(reply->type == REDIS_REPLY_ARRAY){
        std::vector<std::string> vec(reply->elements);
        for(size_t i = 0; i < reply->elements; i++){
            if(reply->element[i]->type == REDIS_REPLY_INTEGER){
                vec[i] = std::to_string(reply->element[i]->integer);
            }else if(reply->element[i]->type == REDIS_REPLY_STRING){
                vec[i] = std::string(reply->element[i]->str,
                                     reply->element[i]->len);

            }else{
                manager->logger->put_error(LOG_ZONE_REDIS,
                        "unknown reply type from redis(array): ",
                        reply->element[i]->type);
                return;
            }
        }
        manager->sub_callback((RedisReplyStringArray){vec});
    }else{
        manager->logger->put_error(LOG_ZONE_REDIS,
                "unknown reply type from redis: ", reply->type);
    }
}
