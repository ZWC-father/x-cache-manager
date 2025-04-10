#include "algo_lfuda_sqlite.h"
#include <cstdint>
    
void callback(std::vector<LFUDA::Cache> cache){
    for(auto it : cache){
        std::cerr << "removing cache: " << it.key << ", size: " <<
        it.size << ", timestamp: " << it.timestamp << ", freq:" <<
        it.freq << ", eff:" << it.eff << std::endl;
    }
}

int main(){
    std::shared_ptr<SQLiteLFUDA> db_sqlite = std::make_shared<SQLiteLFUDA>("./", "cache.db");
    SQLiteLFUDA::MetaLFUDA meta = {0, 100, 0};
    if(db_sqlite->is_new){
        db_sqlite->insert_meta(meta);
    }
    LFUDA lfuda(db_sqlite, callback);
    lfuda.init();
    
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
            LFUDA::Cache cache = {key, size, download_time, hash};
            lfuda.put(cache);
        }else if(opt == "renew"){
            char key[64];
            uint64_t ts;
            scanf("%s%lu", key, &ts);
            lfuda.renew(key, ts);
        }else if(opt == "update"){
            char key[64];
            size_t size;
            scanf("%s%zu", key, &size);
            lfuda.update(key, size);
        }else if(opt == "resize"){
            size_t size;
            scanf("%zu", &size);
            lfuda.resize(size);
        }else if(opt == "query"){
            char key[64];
            scanf("%s", key);
            auto res = lfuda.query(key);
            if(res.key.size()){
                std::cout << "query result: key: " << res.key << ", size: " << res.size <<
                ", hash: " << res.hash[0] << ", download_time: " << res.download_time <<
                ", timestamp: " << res.timestamp << ", freq: " << res.freq <<
                ", eff:" << res.eff << std::endl;
            }
        }else std::cout << "unknown opt: " << str << std::endl;

        lfuda.display();
        std::cout << std::endl;
    }
    return 0;
}
