#include "logging_zones.h"
#include "redis_adapter.h"
#include "logger.h"
#include <chrono>
#include <hiredis/read.h>
#include <memory>
#include <thread>

int main(){
    std::shared_ptr<Logger> logger = std::make_shared<Logger>();

    logger->setup(Logger::LOG_LOGGER_STDOUT | Logger::LOG_LOGGER_STDERR);
    logger->set_level(Logger::LOG_TYPE_CONSOLE,
                      Logger::LOG_LEVEL_DEBUG);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    logger->put_debug(LOG_ZONE_MAIN, "Redis Adapter Test");
    RedisAdapter* redis = new RedisAdapter(logger, 
                                           "127.0.0.1",
                                           6379, "127.0.0.1",
                                           1000, 1000, 30000, 200, 2, true);

    redis->init();
    auto res = redis->command<RedisAdapter::RedisReplyStatus>("SET %s %s", "foo", "bar");
    if(auto* ptr1 = std::get_if<RedisAdapter::RedisReplyStatus>(&res)){
        logger->put_debug(LOG_ZONE_REDIS, "Res: ", ptr1->str);
    }else if(auto* ptr2 = std::get_if<RedisAdapter::RedisReplyError>(&res)){
        logger->put_debug(LOG_ZONE_REDIS, "Error: ", ptr2->str);
    }

    redis->reset();
    
    auto res2 = redis->command<RedisAdapter::RedisReplyStringArray>("MGET %s %s", "yjx", "lhr");
    if(auto* ptr1 = std::get_if<RedisAdapter::RedisReplyStringArray>(&res2)){
        for(auto it : ptr1->vec)logger->put_debug(LOG_ZONE_REDIS, "Res: ", it);
    }else if(auto* ptr2 = std::get_if<RedisAdapter::RedisReplyError>(&res2)){
        logger->put_debug(LOG_ZONE_REDIS, "Error: ", ptr2->str);
    }


    delete redis;

    return 0;

}
