#include "algo_lru_sqlite.h"

LRU::LRU(std::shared_ptr<SQLiteLRU> db, RemoveCallback cb) :
db_sqlite(db), remove_callback(cb),
meta_lru({0, 0, 0}) {}

LRU::~LRU(){
    backup();
}

void LRU::init(){ //data validation should be performed before LRU
    meta_lru = db_sqlite->query_meta();
    
    //some simple check
    if(meta_lru.max_size == 0){
        throw AlgoErrorLRU("init check failed: max_size must not be zero");
    }
    
    auto entry = db_sqlite->query_lru_new();
    if(entry.key.size() && entry.sequence != meta_lru.sequence){
        throw AlgoErrorLRU("init check failed: sequence mismatch");
    }
    if(entry.key.size() && entry.size > meta_lru.cache_size){
        throw AlgoErrorLRU("init check failed: cache_size mismatch");
    }

    if(meta_lru.cache_size > meta_lru.max_size){
        std::cerr << "warning: reach the size limit while initializing, removing..." << std::endl;
        remove_cache(0);
    }
    
    std::cerr << "init finished: " << meta_lru.cache_size << ", " << meta_lru.max_size << ", "
    << meta_lru.sequence << std::endl;
}

void LRU::backup() const{
    db_sqlite->update_meta(meta_lru);
}
//TODO: backup per 100 operations

bool LRU::put(const Cache& cache){
    if(cache.key.empty())throw AlgoErrorLRU("key must not be null");
    if(cache.size == 0 || cache.size > meta_lru.max_size){
        throw AlgoErrorLRU("size must not be zero or greater than max_size");
    }
    
    auto entry = db_sqlite->query_lru_single(cache.key);
    if(entry.key.empty()){
        remove_cache(cache.size);
        
        Cache tmp = cache;
        tmp.sequence = meta_lru.sequence + 1;
        if(!db_sqlite->insert_lru(tmp)){
            throw AlgoErrorLRU("db error: fatal logic error");
        }
        
        meta_lru.sequence++;
        meta_lru.cache_size += tmp.size;
        return 1; 
    }

    std::cerr << "warning: cache already in database: " << cache.key << std::endl;
    //Probable Cause: many concurrent requests to same source for the first time
    
    if(cache.size > entry.size){
        std::cerr << "warning: add duplicate cache, using the larger one" << std::endl;
        update_size(entry, cache.size);
    }

    if(!renew(entry.key)){
        throw AlgoErrorLRU("db error: fatal logic error");
    }

    return 0;
    
}

/*
 * Same Key -> Same MD5 -> Same Path -> Same Name -> Same Cache for Nginx
 * If the cache varies, the metadata in the cached file will indicate it.
 * And Nginx will distinguish it with the ".xxxxxxxxxx" suffix.
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
    if(key.empty())throw AlgoErrorLRU("key must not be null");
    int changed = db_sqlite->update_lru_seq(key, meta_lru.sequence + 1);
    if(changed){
        meta_lru.sequence++;
        return 1;
    }
   
    std::cerr << "warning: no such cache: " << key << std::endl;
    //Probable Cause: the purger does not really purge the cache
    //but it has been removed from database
    return 0; 
   
}
/*
 * We must increase meta.sequence every time! or it will throw
 * an exception due to no row change
 */

bool LRU::update(const std::string& key, size_t new_size){
    if(key.empty())throw AlgoErrorLRU("key must not be null");
    if(new_size == 0 || new_size > meta_lru.max_size){
        throw AlgoErrorLRU("size must not be zero or greater than max_size");
    }
    
    auto entry = db_sqlite->query_lru_single(key);
    if(entry.key.size()){
        update_size(entry, new_size);
        return 1;
    }
       
    std::cerr << "warning: no such cache: " << key << std::endl;
    return 0;
}

void LRU::resize(size_t new_size){
    if(new_size == 0)throw AlgoErrorLRU("max_size must not be zero");
    meta_lru.max_size = new_size;
    remove_cache(0);
}

bool LRU::remove_cache(size_t required, const std::string& mark){
    std::vector<Cache> removed;
    bool flag = 0;
    while(meta_lru.cache_size + required > meta_lru.max_size){
        auto entry = db_sqlite->delete_lru_old();
        if(entry.key.empty()){
            if(removed.size())remove_callback(removed);
            throw AlgoErrorLRU("db error: cache_size mismatch or cache with empty key");
        }

        if(meta_lru.cache_size >= entry.size){
            meta_lru.cache_size -= entry.size;
            removed.push_back(entry);
        }else{
            removed.push_back(entry);
            remove_callback(removed);
            throw AlgoErrorLRU("db error: cache_size mismatch");
        }
        

        flag |= (mark.size() && entry.key == mark);
    }

    if(removed.size())remove_callback(removed);
    return flag;
}

void LRU::update_size(Cache& cache, size_t new_size){ //cache.size will be updated
    if(new_size == cache.size)return;
    if(new_size > cache.size){
        if(remove_cache(new_size - cache.size, cache.key)){
            std::cerr << "warning: the target is removed due to insufficient space, abort updating" << std::endl;
            return;
        }
    }
    
    if(meta_lru.cache_size >= cache.size){
        meta_lru.cache_size -= cache.size;
    }else{
        throw AlgoErrorLRU("db error: cache_size mismatch");
    } 
    meta_lru.cache_size += new_size;
    
    if(!db_sqlite->update_lru_content(cache.key, cache.size = new_size)){
        throw AlgoErrorLRU("db error: fatal logic error");
    }
}

void LRU::display() const{
    std::cerr << "------- status -------\n";
    std::cerr << "total size: " << meta_lru.cache_size << '\n';
    std::cerr << "cache list (most recent first):\n";
    auto data = db_sqlite->query_lru_all();
    for(auto it : data){
        std::cerr << "key: " << it.key << ", size: " << it.size
        << ", sequence: " << it.sequence << std::endl;
    }
    
    backup();
    auto metadata = db_sqlite->query_meta();
    std::cerr << "-------- meta --------\n";
    std::cerr << "max sequence: " << metadata.sequence << ", cache_size: "
    << metadata.cache_size << ", max_size: " << metadata.max_size << std::endl;
}

LRU::Cache LRU::query(const std::string& key) const{
    if(key.empty())throw AlgoErrorLRU("key must not be null");
    auto entry = db_sqlite->query_lru_single(key);
    if(entry.key.empty())std::cerr << "warning: no such cache: " << key << std::endl;
    return entry;
}


