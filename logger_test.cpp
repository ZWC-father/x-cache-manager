#include "logger.h"
#include <thread>
Logger* logger;
void logger_1(){
    for(int i = 0; i <= 1000; i++){
        auto tmp = logger->new_multi();
        tmp->put_warn("test");
        tmp->put_info("yjx");
        tmp->submit();
    }
}

void logger_2(){
    for(int i = 0; i <= 1000; i++){
         logger->put_critical("lhr: ", i);
         logger->put_critical("yjx: ", i);
    }
}

int main(){
    logger = new Logger;
    logger->setup(Logger::LOG_LOGGER_STDOUT |
                  Logger::LOG_LOGGER_FILES,
                  1024, "akioi.log", 512, 3);
    logger->set_level(Logger::LOG_TYPE_CONSOLE | Logger::LOG_TYPE_FILES,
                      Logger::LOG_LEVEL_DEBUG);
    std::thread thr1(logger_1);
    std::thread thr2(logger_2);
    thr1.join();
    thr2.join();

    delete logger;
    return 0;
}
