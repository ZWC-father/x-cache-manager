#include "redis_base.h"
#include "redis_adapter.h"

class RedisLRU{
public:
    struct CacheLRU{
        std::string key;
        size_t size;
        uint64_t download_time;
        std::vector<char> hash;
        int64_t sequence;

        CacheLRU(const std::string& key, size_t size, uint64_t download_time,
                 const std::vector<char> &hash, int64_t sequence = 0) : key(key), size(size),
                 download_time(download_time), hash(hash), sequence(sequence) {}
    };

    struct MetaLRU{
        size_t cache_size, max_size;
        int64_t sequence;
        MetaLRU(size_t cache_size, size_t max_size, int64_t sequence) : cache_size(cache_size),
        max_size(max_size), sequence(sequence) {}
    };


    RedisLRU(const std::shared_ptr<Logger>& logger,
             const std::shared_ptr<RedisAdapter>& redis) :
             redis(redis), logger(logger) {}
   
    void init(){
       auto res1 = redis->command_single<int>("EXISTS LRU::seq");
       auto res2 = redis->command_single<int>("EXISTS LRU::cache");
       auto res3 = redis->command_single<int>("EXISTS LRU::meta");
       if(res1 == 1 && res2 == 1 && res3 == 1){
           is_new = false;
       }else if(res1 == 0 && res2 == 0 && res3 == 0){
           is_new = true;
       }else{
           
       }
       
    }

    const bool is_new;

private:
    std::shared_ptr<RedisAdapter> redis;
    std::shared_ptr<Logger> logger;
    

};
