#include "redis_base.h"
#include <chrono>
#include <ratio>
#include <thread>

void cmd_test(RedisBase* redis, std::string str){
    auto tmp = redis->command_single<std::string>("SET %s %s", str.c_str(), "akioi");
    if(auto t = std::get_if<RedisBase::OtherType>(&tmp)){
        std::cout << "status code: " << t->type << " message: " << t->msg << std::endl;
    }else if(auto t = std::get_if<std::string>(&tmp)){
        std::cout << "reply: " << t << std::endl;
    }
    
}

int main(){
    RedisBase redis("127.0.0.1", 6379, "localhost", 1000, 5000, 30000);
    redis.connect();
    char a[100] = {0};
    redis.reconnect();
    redis.reconnect();

    while(~scanf("%s", a)){
        if(std::string(a) == "exit")break;
        cmd_test(&redis, std::string(a));
    }

//    redis.disconnect();
//    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//    redis.connect();
    return 0;
}
