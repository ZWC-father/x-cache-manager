#include "algo_lfuda_sqlite.h"

LFUDA::LFUDA(std::shared_ptr<SQLiteLFUDA> db, RemoveCallback cb) :
db_sqlite(db), remove_callback(cb),
meta_lfuda(0, 0, 0){}

LFUDA::~LFUDA(){
    backup();
}

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
    
    std::cerr << "warning: renew this cache" << std::endl;
    if(!renew(entry.key, cache.timestamp)){
        throw AlgoErrorLFUDA("db error: fatal logic error");
    }
    
    if(cache.size > entry.size){
        std::cerr << "warning: duplicate cache, using the larger one" << std::endl;
        update_size(entry, cache.size);
    }

    return 0;
    
}   

bool LFUDA::renew(const std::string& key, uint64_t timestamp){
    if(key.empty())throw AlgoErrorLFUDA("key must not be null");
    
    auto entry = db_sqlite->query_lfuda_time(key);
    if(entry.key.size()){
        if(entry.timestamp > timestamp){
            std::cerr << "warning: invalid timestamp: earlier than last access" << std::endl;
            timestamp = entry.timestamp;
        }
        
        entry.eff = ++entry.freq + meta_lfuda.global_aging;
        if(!db_sqlite->update_lfuda_tfe(key, timestamp, entry.freq, entry.eff)){
            throw AlgoErrorLFUDA("db error: fatal logic error");
        }
        return 1;
    }

    std::cerr << "warning: no such cache: " << key << std::endl;
    return 0;
}

bool LFUDA::update(const std::string& key, size_t new_size){
    if(key.empty())throw AlgoErrorLFUDA("cache_key must not be null");
    if(new_size == 0 || new_size > meta_lfuda.max_size){
        throw AlgoErrorLFUDA("size must not be zero or greater than max_size");
    }
    
    auto entry = db_sqlite->query_lfuda_single(key);
    if(entry.key.size()){
        update_size(entry, new_size);
        return 1;
    }

    std::cerr << "waring: no such cache: " << key << std::endl;
    return 0;
}

void LFUDA::resize(size_t new_size){
    if(new_size == 0)throw AlgoErrorLFUDA("max_size must not be zero");
    meta_lfuda.max_size = new_size;
    remove_cache(0);
}

bool LFUDA::remove_cache(size_t required, const std::string& mark){
    if(meta_lfuda.cache_size + required <= meta_lfuda.max_size)return 0;
    std::vector<Cache> removed;
    bool flag = 0;
    while(meta_lfuda.cache_size + required > meta_lfuda.max_size){
        auto entry = db_sqlite->delete_lfuda_old();
        
        if(entry.key.empty()){
            if(removed.size())remove_callback(removed);
            throw AlgoErrorLFUDA("db error: cache_size mismatch or cache with empty key");
        }
        
        if(meta_lfuda.cache_size >= entry.size){
            meta_lfuda.cache_size -= entry.size;
            meta_lfuda.global_aging = entry.eff;
            removed.push_back(entry);
        }else{
            removed.push_back(entry);
            remove_callback(removed);
            throw AlgoErrorLFUDA("db error: cache_size mismatch");
        }

        flag |= (entry.key == mark);
    }
    
    if(removed.size())remove_callback(removed);
    return flag;
}

void LFUDA::update_size(Cache& cache, size_t new_size){
    if(new_size == cache.size)return;
    if(new_size > cache.size){
        if(remove_cache(new_size - cache.size, cache.key)){
            std::cerr << "warning: the target is removed due to insufficient space, abort updating" << std::endl;
            return;
        }
    }

    if(meta_lfuda.cache_size >= cache.size){
        meta_lfuda.cache_size -= cache.size;
    }else{
        throw AlgoErrorLFUDA("db error: cache_size mismatch");
    }
    meta_lfuda.cache_size += new_size;

    if(!db_sqlite->update_lfuda_content(cache.key, cache.size = new_size)){
        throw AlgoErrorLFUDA("db error: fatal logic error");
    }
}

void LFUDA::display() const{
    std::cerr << "--- status (best first) ---\n";
    std::cerr << "cache list:\n";
    auto data = db_sqlite->query_lfuda_all();
    for(auto it : data){
        std::cerr << "key: " << it.key << ", size: " << it.size << ", timestamp: "
        << it.timestamp << ", freq: " << it.freq << ", eff: " << it.eff << '\n';
    }
    
    backup();
    auto metadata = db_sqlite->query_meta();
    std::cerr << "---------- meta ----------\n";
    std::cerr << "cache_size: " << metadata.cache_size << ", max_size: " <<
    metadata.max_size << ", global_aging: " << metadata.global_aging << std::endl;

}

LFUDA::Cache LFUDA::query(const std::string& key) const{
    if(key.empty())throw AlgoErrorLFUDA("key must not be null");
    auto entry = db_sqlite->query_lfuda_single(key);
    if(entry.key.empty())std::cerr << "warning: no such cache: " << key << std::endl;
    return entry;
}
