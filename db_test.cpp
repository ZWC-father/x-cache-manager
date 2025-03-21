#include "db_sql3.h"
#include <utility>

int main(int argc, char** argv){
    std::vector<char>hash(0xff);
    DB* db = new DB(std::string("."), std::string("cache.db"), LRU);
    DB::Cache cache(std::in_place_type<DB::CacheLRU>, 
                    DB::CacheLRU{std::string("Test"), 16,
                    0, hash, 1145});
    db->insert(cache);
//    std::get<DB::CacheLRU>(cache).key = std::string("test2");
//    db.insert(cache);
//    db.update(std::string("Test"), 10);

    return 0;

}
