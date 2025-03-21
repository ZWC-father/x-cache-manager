#include "algo_lru.h"
#include <unordered_map>

class LRU_Internal : public LRU_Interface{
    LRU_Internal(size_t size) : max_size(size), cache_size(0) {
    }
    void init() override{}
    size_t size() const override{
        return cache_list.size();
    }
    LRU::Cache put_remove(std::string& key, size_t size, Remo)const override{
        auto it = cache_map.find(key);
        if(it == cache_map.end())return {};
        
    }
    LRU
private:
    size_t max_size, cache_size;
    std::list<LRU::Cache> cache_list;
    std::unordered_map<std::string, std::list<LRU::Cache>::iterator> cache_map;
};
