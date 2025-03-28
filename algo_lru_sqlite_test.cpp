#include "algo_lru_sqlite.h"
#include <memory>

void callback(std::vector<LRU::Cache> cache){
    for(auto it : cache){
        std::cerr << "removing cache: " << it.key << " with size: " <<
        it.size << " and sequence: " << it.sequence << std::endl;
    }
}

int main(){
    std::shared_ptr<SQLiteLRU> db_sqlite = std::make_shared<SQLiteLRU>("./", "cache.db");
    SQLiteLRU::MetaLRU meta = {0, 100, 0};
    if(db_sqlite->is_new){
        db_sqlite->insert_meta(meta);
    }
    LRU lru(db_sqlite, callback);
    char str[64] = {};

    while(~scanf("%s", str)){
        std::string opt(str);
        if(opt == "exit")break;
        else if(opt == "new"){
        }
    }
    return 0;
}
