#include "algo_lru_sqlite.h"

void callback(std::vector<LRU::Cache> cache){
    for(auto it : cache){
        std::cerr << "removing cache: " << it.key << ", size: " <<
        it.size << ", sequence: " << it.sequence << std::endl;
    }
}

int main(){
    std::shared_ptr<SQLiteLRU> db_sqlite = std::make_shared<SQLiteLRU>("./", "cache.db");
    SQLiteLRU::MetaLRU meta = {0, 100, 0};
    if(db_sqlite->is_new){
        db_sqlite->insert_meta(meta);
    }
    LRU lru(db_sqlite, callback);
    lru.init();
    
    char str[64] = {};
    std::vector<char> hash;
    hash.push_back(0x3f);
    hash.push_back(0x7f);
    uint64_t download_time = 170000000;

    while(~scanf("%s", str)){
        std::string opt(str);
        if(opt == "exit")break;
        else if(opt == "put"){
            char key[64];
            size_t size;
            scanf("%s%zu", key, &size);
            LRU::Cache cache = {key, size, download_time, hash};
            lru.put(cache);
        }else if(opt == "renew"){
            char key[64];
            scanf("%s", key);
            lru.renew(key);
        }else if(opt == "update"){
            char key[64];
            size_t size;
            scanf("%s%zu", key, &size);
            lru.update(key, size);
        }else if(opt == "resize"){
            size_t size;
            scanf("%zu", &size);
            lru.resize(size);
        }else if(opt == "query"){
            char key[64];
            scanf("%s", key);
            auto res = lru.query(key);
            if(res.key.size()){
                std::cout << "query result: key: " << res.key << " size: " << res.size <<
                " hash: " << res.hash[0] << " download_time: " << res.download_time <<
                " sequence: " << res.sequence << std::endl;
            }
        }else std::cout << "unknown opt: " << str << std::endl;

        lru.display();
        std::cout << std::endl;
    }
    return 0;
}
