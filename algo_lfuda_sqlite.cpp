#include "algo_lfuda_sqlite.h"
#include <memory>

LFUDA::LFUDA(std::shared_ptr<SQLiteLFUDA> db, RemoveCallback cb) :
db_sqlite(db), remove_callback(cb),
meta_lfuda(0, 0, 0){}

void LFUDA::init(){
    meta_lfuda = db_sqlite->query_meta();

    //some simple check
    if(meta_lfuda.max_size == 0){
        throw AlgoErrorLFUDA("db error(init check): max_size must not be zero");
    }

    auto entry = db_sqlite->query_lfuda_worst();
    if(entry.key.empty() && meta_lfuda.cache_size){
        throw AlgoErrorLFUDA("db error(init check): empty cache but cache_size mismatch");
    }

    if(entry.key.size() && entry.size > meta_lfuda.cache_size){
        throw AlgoErrorLFUDA("db error(init check): cache_size mismatch");
    }

    if(meta_lfuda.cache_size > meta_lfuda.max_size){
        std::cerr << "warning: reach the size limit while initializing, removing..." << std::endl;
        remove_cache(0);
    }

    std::cerr << "init finished: " << meta_lfuda.cache_size << ", "
    << meta_lfuda.max_size << ", " << meta_lfuda.global_aging << std::endl;
}

void LFUDA::backup() const{
    db_sqlite->update_meta(meta_lfuda);
}
//TODO: backup per 100 operations

bool LFUDA::put(const Cache& cache){
    if(cache.key.empty())throw AlgoErrorLFUDA("cache_key must not be null");
    if(cache.size == 0 || cache.size > meta_lfuda.max_size){
        throw AlgoErrorLFUDA("size must not be zero or greater than max_size");
    }

    auto entry = db_sqlite->query_lfuda_single(cache.key);
    if(entry.key.empty()){
        remove_cache(cache.size);

        Cache tmp = cache;
        tmp.freq = 1, tmp.eff = meta_lfuda.global_aging + 1;
        if(!db_sqlite->insert_lru(tmp)){
            throw AlgoErrorLFUDA("db error: fatal logic error");
        }

        meta_lfuda.cache_size += tmp.size;
        return 1;
    }

    std::cerr << "warning: cache already in database: " << cache.key << std::endl;
    
    if(cache.size > entry.size){
        std::cerr << "warning: add duplicate cache, using the larger one" << std::endl;
        update_size(entry, cache.size);
    }

    if(!renew(entry.key, cache.timestamp, )){
        throw AlgoErrorLFUDA("db error: fatal logic error");
    }

    return 0;
    
}   

bool LFUDA::renew(const std::string& key, uint64_t timestamp){
    if(key.empty())throw AlgoErrorLFUDA("key must not be null");
    auto entry = db_sqlite->query_lfuda_time(key);
    if(entry.key.size()){
        if(entry.timestamp <= timestamp){
            std::cerr << "warning: invalid timestamp: earlier than last access" << std::endl;
            timestamp = entry.timestamp;
        }
        int changed = db_sqlite->update_lfuda_tfe(key, timestamp, uint64_t freq, uint64_t eff)
    }

    if(entry.key.size()){
        update_size()
        return 1;
    }

    std::cerr << "warning: no such cache: " << key << std::endl;
    return 0;
}

bool LFUDA::update(const std::string& key, size_t size){
    if(new_size == 0 || new_size > meta_lfuda.max_size){
        throw AlgoErrorLFUDA("size must not be zero or greater than max_size");
    }
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
