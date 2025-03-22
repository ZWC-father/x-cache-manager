#include "db_sqlite_lru.h"

int main(int argc, char** argv){
    std::vector<char>tmp;
    tmp.push_back(40);
    SQLiteLRU* db = new SQLiteLRU(std::string("."), std::string("cache.db"));
    SQLiteLRU::CacheLRU cache1 = {"test-1",  114, 514, tmp, 1};
    SQLiteLRU::CacheLRU cache2 = cache1;
    cache2.key = "test-2";
    cache2.sequence = 0;
    
    std::cout << db->insert(cache1) << std::endl;
    std::cout << db->insert(cache2) << std::endl;
    std::vector<SQLiteLRU::CacheLRU> data = db->query_all();
    for(auto it : data){
        std::cout << it.key << "  " << it.sequence << "  " << it.size << "  " << it.download_time << std::endl;
        for(auto iter : it.hash)std::cout << iter << ' ';
        std::cout << "----------" << std::endl;
    }
    
    std::cout << db->update_content("test-1", 250) << std::endl;
    std::cout << db->count("test-2") << std::endl;
    
    data = db->query_all();
    for(auto it : data){
        std::cout << it.key << "  " << it.sequence << "  " << it.size << "  " << it.download_time << std::endl;
        for(auto iter : it.hash)std::cout << iter << ' ';
        std::cout << "----------" << std::endl;
    }

    SQLiteLRU::CacheLRU cache3 = cache2;
    cache3.key = "test-2";
    cache3.sequence = 10;
    db->insert(cache3);
    SQLiteLRU::CacheLRU del = db->delete_old();
    std::cout << del.key << std::endl;
    
    del = db->delete_any("test-2");
    
    


//    std::get<DB::CacheLRU>(cache).key = std::string("test2");
//    db.insert(cache);
//    db.update(std::string("Test"), 10);

    return 0;

}
