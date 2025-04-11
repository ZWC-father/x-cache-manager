#include "logger.h"
#include "redis_base.h"
#include <memory>

class RedisAdapter{
public:
    RedisAdapter(const std::string& h, int p, const std::string& src,
                 int con_t, int cmd_t, int al_int, int rt_int,
                 int max_rt, std::shared_ptr<Logger> l){
        redis = std::make_unique<RedisBase>(h, p, src, con_t, cmd_t,
                                                al_int, rt_int, max_rt);
        logger = l;

    } 
    
    ~RedisAdapter(){
        redis->disconnect();
    }
    
    void init(){
        redis = std::make_unique<RedisBase>(host, port, src_addr,
        connect_timeout, command_timeout, alive_interval,
        retry_interval, max_retries);
        redis->connect();
    }

    void reset(){
        redis.reset();
        init();
    }

    void close(){
        redis->disconnect();
    }

    template<typename T, typename... Args>
    std::variant<RedisBase::OtherType, T> command(const char* cmd, Args&&... args){
        redis->command_single<T>(cmd, std::forward<Args>(args)...);
    }
    
private:
    const std::string host, src_addr;
    int port;
    int connect_timeout, command_timeout;
    int alive_interval, retry_interval, max_retries;

    std::shared_ptr<Logger> logger;
    std::unique_ptr<RedisBase> redis;
    
};
