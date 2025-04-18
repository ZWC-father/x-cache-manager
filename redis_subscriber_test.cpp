#include "redis_subscriber.h"
#include "logger.h"
#include "logging_zones.h"
#include <chrono>
#include <memory>
#include <thread>
std::shared_ptr<Logger> logger = std::make_shared<Logger>();
void cb(SubManager::Reply reply){
    if(auto *ptr = std::get_if<SubManager::RedisReplyError>(&reply)){
        logger->put_debug(LOG_ZONE_MAIN, "error from callback: ", ptr->str);
    }else if(auto *ptr = std::get_if<SubManager::RedisReplyStatus>(&reply)){
        logger->put_debug(LOG_ZONE_MAIN, "status from callback: ", ptr->str);
    }else if(auto *ptr = std::get_if<SubManager::RedisReplyStringArray>(&reply)){
        for(auto it : ptr->vec){
            logger->put_debug(LOG_ZONE_REDIS, "message from callback: ", it);
        }
    }
}
int main(){
    logger->setup(Logger::LOG_LOGGER_STDOUT);
    logger->set_level(Logger::LOG_TYPE_CONSOLE, Logger::LOG_LEVEL_DEBUG);

    SubManager* manager = new SubManager(cb, logger, "127.0.0.1");
    manager->init();
    std::thread thr([manager]{
            manager->connect();
            logger->put_debug(LOG_ZONE_MAIN, "connection thread exit");
            });
    thr.detach();
    while(manager->status != 1){
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    logger->put_info(LOG_ZONE_MAIN, "sub status: ", manager->subscribe());
    std::this_thread::sleep_for(std::chrono::milliseconds(40000));
    manager->uninit();
    delete manager;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    return 0;
}
