#include "algo_lfuda.h"

LFUDA::LFUDA(size_t size, RemoveCallback cb) :
             aging(0), max_size(size), cache_size(0), remove_callback(cb){
    if(size == 0)throw AlgoErrorLFUDA("max_size must not be zero");
}

void LFUDA::init(std::vector<Cache>&& caches, uint64_t age){
    while(caches.size()){
        cache_map[caches.back().key] = cache_bst.emplace(caches.back()).first;
        cache_size += caches.back().size;
        caches.pop_back();
    }
    aging = age;
}

std::pair<std::vector<LFUDA::Cache>, uint64_t> LFUDA::backup(){
    std::vector<Cache> caches;
    cache_map.clear();
    cache_size = 0;
    while(cache_bst.size()){
        caches.push_back(*cache_bst.begin());
        cache_bst.erase(cache_bst.begin());
    }
    return std::make_pair(caches, aging);
}

bool LFUDA::put(const Cache& cache){
    if(cache.key.empty())throw AlgoErrorLFUDA("cache_key must not be null");
    if(cache.size == 0){
        std::cerr << "cache_size must not be zero" << std::endl;
        return 0;
    }

    auto it = cache_map.find(cache.key);
    if(it == cache_map.end()){
        remove_cache(cache.size);
        cache_size += cache.size;
        cache_map[cache.key] = cache_bst.emplace([&]{
            Cache tmp = cache;
            tmp.freq = 1, tmp.eff = aging + 1;
            return tmp;
        }()).first;
        return 1;
    }

    std::cerr << "warning: cache already exist" << std::endl;
    
    if(cache.size > it->second->size){
        std::cerr << "warning: size mismatch, using the larger one" << std::endl;
        cache_map[cache.key] = update_size(it->second, cache.size);
    }
    return 0;
    
}   

bool LFUDA::renew(const std::string& key, uint64_t timestamp){
    auto it = cache_map.find(key);
    if(it != cache_map.end()){
        auto tmp = cache_bst.extract(it->second);
        tmp.value().eff = ++tmp.value().freq + aging;
        tmp.value().timestamp = timestamp;
        cache_map[key] = cache_bst.insert(std::move(tmp)).position;
        return 1;
    }

    std::cerr << "warning: no such cache: " << key << std::endl;
    return 0;
}

bool LFUDA::update(const std::string& key, size_t size){
    auto it = cache_map.find(key);
    if(it != cache_map.end()){
        if(size != it->second->size){
            auto tmp = update_size(it->second, size);
            cache_map[key] = tmp;
        }
        return 1;
    }

    std::cerr << "waring: no such cache: " << key << std::endl;
    return 0;
}

void LFUDA::resize(size_t new_size){
    if(new_size == 0)throw AlgoErrorLFUDA("max_size must not be zero");
    max_size = new_size;
    remove_cache(0);
}

void LFUDA::remove_cache(size_t required){
    std::vector<Cache> removed;
    while(cache_map.size() && cache_size + required > max_size){
        auto del_it = cache_bst.begin();
        aging = del_it->eff;
        cache_size -= del_it->size;
        removed.push_back(*del_it);
        cache_map.erase(del_it->key);
        cache_bst.erase(del_it);
    }
    
    if(cache_map.empty())std::cerr << "no cache remained" << std::endl;
    if(removed.size())remove_callback(removed);
}

LFUDA::Iter LFUDA::update_size(Iter it, size_t size){
    if(size > it->size)remove_cache(size - it->size);
        
    cache_size -= it->size;
    cache_size += size;
    auto tmp = cache_bst.extract(it);
    tmp.value().size = size;
        
    return cache_bst.insert(std::move(tmp)).position;
    
}

void LFUDA::display() const{
    std::cerr << "------- status -------\n";
    std::cerr << "total size: " << cache_size << '\n';
    std::cerr << "global aging factor: " << aging << '\n';
    std::cerr << "cache list (most popular first):\n";
    for(auto it = cache_bst.rbegin(); it != cache_bst.rend(); it++){
        std::cerr << "key: " << it->key << " size: " << it->size << " timestamp: "
        << it->timestamp << " freq: " << it->freq << " eff_freq: " << it->eff << '\n';
    }
    std::cerr << std::endl;
}

LFUDA::Cache LFUDA::query(const std::string& key) const{
    auto it = cache_map.find(key);
    if(it == cache_map.end())return {};
    return *it->second;
}
