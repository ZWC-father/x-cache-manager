#include "redis_base.h"
#include <chrono>
#include <ratio>
#include <thread>

int main(){
    RedisBase redis("127.0.0.1", 6379, "localhost", 1000, 1000, 30000);
    redis.connect();
    auto tmp = redis.command_single<std::string>("SET %s %s", "yjx", "akioi");
    if(auto t = std::get_if<RedisBase::OtherType>(&tmp)){
        std::cout << "status code: " << t->type << " message: " << t->msg << std::endl;
    }else if(auto t = std::get_if<std::string>(&tmp)){
        std::cout << "reply: " << t << std::endl;
    }
    redis.disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    redis.connect();
    return 0;
}
