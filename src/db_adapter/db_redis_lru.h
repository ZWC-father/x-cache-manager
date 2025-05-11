#include "redis_base.h"
#include "redis_adapter.h"
#include <optional>
#include <variant>

class RedisLRU{
public:
    using Int = RedisAdapter::RedisReplyInteger;
    using String = RedisAdapter::RedisReplyString;
    using Status = RedisAdapter::RedisReplyStatus;
    
    struct CacheLRU{
        std::string key;
        size_t size;
        uint64_t download_time;
        std::string hash;
        int64_t sequence;

        CacheLRU(const std::string& key, size_t size, uint64_t download_time,
                 const std::string &hash, int64_t sequence = 0) : key(key), size(size),
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
             redis(redis), logger(logger) {
        auto res = redis->command_single<int>("EXISTS lru_meta");
        if(!std::holds_alternative<int>(res)){
            throw RedisError("unknown redis error");
        }

        if(std::get<int>(res) == 1){
            _new = false;
        }else{
            _new = true;
        }
    }

    bool is_new() const {return _new;}

    void insert_meta(const MetaLRU& meta){
        auto res = redis->command_single<Int>("HSET lru_meta cache_size %s", std::to_string(meta.cache_size));
        check_error(res);
        if(check_value<Int>(res, 0))throw RedisError("meta already exists");
    }
   
    bool insert_lru(const CacheLRU& entry){
        std::string serial = RedisAdapter::serialize(entry.size, entry.download_time, entry.hash);
        auto res = redis->command_single<Int>("EXISTS %s", entry.key);
        check_error(res);
        return check_value<Int>(res, 0);

        auto res2 = redis->command_single<Int>("HSET %s %s", entry.key, serial);
        check_error(res2);
        return check_value<Int>(res2, 1);
    }

    int update_lru(const CacheLRU& entry){
        
    }

    bool _new;

private:
    std::shared_ptr<RedisAdapter> redis;
    std::shared_ptr<Logger> logger;

    void check_error(const auto& res){
        if(std::holds_alternative<RedisAdapter::RedisReplyError>(res)){
            throw RedisError("redis command error: " + std::get<RedisAdapter::RedisReplyError>(res).str);
        }
    }

    template<typename T>
    bool check_value(const auto& res, T expection){
        if(auto *ptr = std::get_if<T>(&res)){
            return *ptr == expection; 
        }
        return false;
    }

    template<typename T>
    std::optional<T> get_value(const auto& res){
        if(auto *ptr = std::get_if<T>(res))return *ptr;
        return std::nullopt;
    }
    
   
};
