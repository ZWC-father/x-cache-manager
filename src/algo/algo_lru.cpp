#include "algo_lru.h"

LRU::LRU(size_t size, RemoveCallback cb) :
         max_size(size), cache_size(0), remove_callback(cb){
    if(max_size == 0)throw AlgoErrorLRU("max_size must not be zero");
}

void LRU::init(std::vector<Cache>&& caches){
    while(caches.size()){
        cache_list.push_front(caches.back());
        cache_map[caches.back().key] = cache_list.begin();
        cache_size += caches.back().size;
        caches.pop_back();
    }
    if(cache_size > max_size){
        std::cerr << "warning: reach the size limit while init" << std::endl;
//      remove_cache(0);
    }
}

std::vector<LRU::Cache> LRU::backup(){
    std::vector<Cache> caches;
    cache_map.clear();
    cache_size = 0;
    while(cache_list.size()){
        caches.push_back(cache_list.front());
        cache_list.pop_front();
    }
    return caches;
}

bool LRU::put(const Cache& cache){
    if(cache.key.empty())throw AlgoErrorLRU("cache_key must not be null");
    if(cache.size == 0){
        std::cerr << "warning: size should not be zero, ignore..." << std::endl;
        return 0;
    }
    
    auto it = cache_map.find(cache.key);
    if(it == cache_map.end()){
        remove_cache(cache.size);
        cache_size += cache.size;
        cache_list.push_front(cache);
        cache_map[cache.key] = cache_list.begin();

        return 1;
    }
       
    std::cerr << "warning: cache already in database" << std::endl;
    //Probable Cause: many concurrent download from same source for the first time
    
    if(cache.size > it->second->size){
        std::cerr << "warning: add duplicate cache, using the larger one" << std::endl;
        update_size(it->second, cache.size);
        
    }
    return 0;
    
}

/*
 * Same Key -> Same MD5 -> Same Path -> Same Name -> Same Cache for Nginx
 * If the cache varies, the metadata in the cached file will indicate it.
 * And Nginx will distinguish it with the ".xxxxxxxxxxx" suffix.
 * We will regard this kind of file as GARBAGE, and delete it.
 * Considering the concurrrent downloading situation, we prevent deleting 
 * the file that is opened by some process.
 * 
 * It was very important that you shoule configure the "hash key" very carefully,
 * as it is the only standard to differ the caches.
 *
 * Every cache marked as duplicated will be cleared by GC Process!
 */

bool LRU::renew(const std::string& key){
    auto it = cache_map.find(key);
    if(it != cache_map.end()){
        cache_list.splice(cache_list.begin(), cache_list, it->second);
        return 1;
    }
    std::cerr << "warning: no such cache: " << key << std::endl;
    //Probable Cause: the purger does not really purge the cache yet
    return 0;
   
}

bool LRU::update(const std::string& key, size_t size){
    auto it = cache_map.find(key);
    if(it == cache_map.end()){
        if(size != it->second->size){
            update_size(it->second, size);
        }
        return 1;
    }
       
    std::cerr << "warning: no such cache: " << key << std::endl;
    return 0;
}

void LRU::resize(size_t new_size){
    if(new_size == 0)throw AlgoErrorLRU("max_size must not be zero");
    max_size = new_size;
    remove_cache(0);
}

void LRU::remove_cache(size_t required){
    std::vector<Cache> removed;
    while(cache_list.size() && cache_size + required > max_size){
        auto del_it = cache_list.end();
        del_it--;
        cache_map.erase(del_it->key);
        cache_size -= del_it->size;
        removed.push_back(*del_it);
        cache_list.pop_back();
    }

    if(cache_map.empty())std::cerr << "no cache remained" << std::endl;
    if(removed.size())remove_callback(removed);
}

void LRU::update_size(Iter it, size_t size){
    if(size > it->size)remove_cache(size - it->size);
    cache_size -= it->size;
    cache_size += size;
    it->size = size;
}

void LRU::display() const{
    std::cerr << "------- status -------\n";
    std::cerr << "total size: " << cache_size << '\n';
    std::cerr << "cache list (most recent first):\n";
    for(auto it = cache_list.begin(); it != cache_list.end(); it++){
        std::cerr << "key: " << it->key << " size: " << it->size << '\n';
    }
    std::cerr << std::endl;
}

LRU::Cache LRU::query(const std::string& key) const{
    auto it = cache_map.find(key);
    if(it == cache_map.end())return {};
    return *it->second;
}
