#include "algo_lru_sqlite.h"
#include <memory>

LRU::LRU(std::shared_ptr<SQLiteLRU> db, RemoveCallback cb) :
db_sqlite(db), remove_callback(cb),
meta_lru({0, 0, 0}) {}

LRU::~LRU(){
    backup();
}

void LRU::init(){ //data validation should before the construction of LRU
    meta_lru = db_sqlite->query_meta();
    if(meta_lru.max_size == 0)throw AlgoErrorLRU("max_size must not be zero");
    if(meta_lru.cache_size > meta_lru.max_size){
        std::cerr << "warning: reach the size limit while init, removing..." << std::endl;
        remove_cache(0);
    }
}

void LRU::backup(){
    db_sqlite->update_meta(meta_lru);
}

bool LRU::put(const Cache& cache){
    if(cache.key.empty())throw AlgoErrorLRU("cache_key must not be null");
    if(cache.size == 0){
        std::cerr << "warning: size should not be zero, ignore..." << std::endl;
        return 0;
    }
    
    auto entry = db_sqlite->query_one(cache.key);
    if(entry.key.empty()){
        Cache tmp = cache;
        remove_cache(tmp.size);
        meta_lru.cache_size += tmp.size;
        tmp.sequence = ++meta_lru.sequence;
        if(!db_sqlite->insert(tmp)){
            throw AlgoErrorLRU("sqlite unknonw error");
        }
        return 1; 
    }

    std::cerr << "warning: cache already in database" << std::endl;
    //Probable Cause: many concurrent download from same source for the first time
    
    if(cache.size > entry.size){
        std::cerr << "warning: add duplicate cache, using the larger one" << std::endl;
        update_size(entry, cache.size);
        if(!renew(entry.key)){
            throw AlgoErrorLRU("sqlite unknonw error");
        }
        
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
    if(key.empty())throw AlgoErrorLRU("cache_key must not be null");
    int changed = db_sqlite->update_seq(key, ++meta_lru.sequence);
    if(!changed){
        std::cerr << "warning: no such cache: " << key << std::endl;
        //Probable Cause: the purger does not really purge the cache yet
        return 0; 
    }
    return 1;
   
}

bool LRU::update_content(const std::string& key, size_t new_size){
    auto entry = db_sqlite->query_one(key);
    if(entry.key.size()){
        update_size(entry, new_size);
        return 0;
    }
       
    std::cerr << "warning: no such cache: " << key << std::endl;
    return 0;
}

void LRU::resize(size_t new_size){
    if(new_size == 0)throw AlgoErrorLRU("max_size must not be zero");
    meta_lru.max_size = new_size;
    remove_cache(0);
}

void LRU::remove_cache(size_t required){
    std::vector<Cache> removed;
    while(meta_lru.cache_size + required > meta_lru.max_size){
        auto entry = db_sqlite->delete_old();
        if(entry.key.empty()){
            if(entry.sequence != meta_lru.max_size)throw AlgoErrorLRU("sqlite unknonw error");
            std::cerr << "no cache remained" << std::endl;
        }
        meta_lru.cache_size -= entry.size;
        removed.push_back(entry);
    }

    if(removed.size())remove_callback(removed);
}

void LRU::update_size(Cache& cache, size_t new_size){
    if(new_size == cache.size)return;
    if(new_size > cache.size)remove_cache(new_size - cache.size);
    meta_lru.cache_size += new_size;
    meta_lru.cache_size -= cache.size;
    if(!db_sqlite->update_content(cache.key, cache.size = new_size)){
        throw AlgoErrorLRU("sqlite unknonw error");
    }
}

void LRU::display() const{
    std::cerr << "------- status -------\n";
    std::cerr << "total size: " << meta_lru.cache_size << '\n';
    std::cerr << "cache list (most recent first):\n";
    auto data = db_sqlite->query_all();
    for(auto it : data){
        std::cerr << "key: " << it.key << " size: " << it.size
        << " sequence: " << it.sequence << std::endl;
    }
}

LRU::Cache LRU::query(const std::string& key) const{
    return db_sqlite->query_one(key);
}
