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
    auto res = redis->command_single<RedisAdapter::RedisReplyStatus>("SEOHKOLINT %s %s", "foo", "bar");
    if(auto* ptr1 = std::get_if<RedisAdapter::RedisReplyStatus>(&res)){
        logger->put_debug(LOG_ZONE_REDIS, "Res: ", ptr1->str);
    }else if(auto* ptr2 = std::get_if<RedisAdapter::RedisReplyError>(&res)){
        logger->put_debug(LOG_ZONE_REDIS, "Error: ", ptr2->str);
    }else if(auto* ptr3 = std::get_if<RedisAdapter::RedisReplyNil>(&res)){
        logger->put_debug(LOG_ZONE_MAIN, "Res: (nil)");
    }

    redis->reset();
    
    auto res2 = redis->command_multi<RedisAdapter::RedisReplyString>("MGET %s %s", "yjx", "byq");
    for(auto it : res2){
        if(auto* ptr1 = std::get_if<RedisAdapter::RedisReplyString>(&it)){
            logger->put_debug(LOG_ZONE_MAIN, "Res: ", ptr1->str);
        }else if(auto* ptr2 = std::get_if<RedisAdapter::RedisReplyError>(&it)){
            logger->put_debug(LOG_ZONE_MAIN, "Error: ", ptr2->str);
        }else if(auto* ptr3 = std::get_if<RedisAdapter::RedisReplyNil>(&it)){
            logger->put_debug(LOG_ZONE_MAIN, "Res: (nil)");
        }

    }


    delete redis;

    return 0;

}
