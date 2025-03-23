#include <cstdint>
#include <tuple>
#include <vector>
#include <iostream>
#include <variant>
#include <tuple>
#include <stdexcept>
#include "sqlite_base.h"
#include "sql_statement.h"

class SQLiteLRU : public SQLiteBase {
public:
    struct CacheLRU{
        std::string key;
        size_t size;
        uint64_t download_time;
        std::vector<char> hash;
        int64_t sequence;
        
        CacheLRU(const std::string& key, size_t size, uint64_t download_time,
                 const std::vector<char> hash, int64_t sequence) : key(key), size(size),
                 download_time(download_time), hash(hash), sequence(sequence) {}
    };

    struct MetaLRU{
        size_t cache_size, max_size;
        int64_t sequence;
        MetaLRU(size_t cache_size, size_t max_size, int64_t sequence) : cache_size(cache_size),
        max_size(max_size), sequence(sequence) {}
    };
    
    SQLiteLRU(const std::string& work_dir, const std::string& db_name) :
              SQLiteBase(work_dir, db_name){
        if(open())execute(SQL_CHECK_LRU), execute(SQL_CHECK_METALRU);
        else execute(SQL_CREATE_LRU), execute(SQL_CREATE_METALRU);
        execute(SQL_CREATE_INDEX_LRU);
    }
    
    int insert(const CacheLRU& entry){ //
        return execute(SQL_INSERT_LRU, entry.key, entry.size, entry.download_time,
                entry.hash, entry.sequence);
    }

    int insert_meta(const MetaLRU& entry){
        return execute(SQL_INSERT_METALRU, entry.cache_size, entry.max_size, entry.sequence);
    }

    int update_content(const std::string& key, size_t size){
        return execute(SQL_UPDATE_LRU_CONTENT, size, key);
    }

    int update_meta(const MetaLRU& entry){
        return execute(SQL_UPDATE_METALRU, entry.cache_size, entry.max_size, entry.sequence);
    } 

    int count(const std::string& key){
        return query_num(SQL_QUERY_NUM_LRU, key);
    }

    CacheLRU query_old(){
        auto raw_entry = query_single<std::string, size_t,
                                      uint64_t, std::vector<char>,
                                      int64_t>(SQL_QUERY_OLD_LRU);
        return std::make_from_tuple<CacheLRU>(raw_entry);
    }

    std::vector<CacheLRU> query_all(){
        auto raw_data = query<std::string, size_t, uint64_t,
                              std::vector<char>, int64_t>(SQL_QUERY_ALL_LRU);
        std::vector<CacheLRU> data;//add memory optimization
        while(raw_data.size()){
            data.push_back(std::make_from_tuple<CacheLRU>(raw_data.back()));
            raw_data.pop_back();
        }
        return data;
    }

    MetaLRU query_meta(){
        auto raw_entry = query_single<size_t, size_t,
                                      int64_t>(SQL_QUERY_METALRU);
        return std::make_from_tuple<MetaLRU>(raw_entry);
    }
    
    CacheLRU delete_old(){
        auto raw_entry = query_single<std::string, size_t,
                                      uint64_t, std::vector<char>,
                                      int64_t>(SQL_DELETE_OLD_LRU);
        return std::make_from_tuple<CacheLRU>(raw_entry);
    }
    
    CacheLRU delete_any(const std::string& key){
        auto raw_entry = query_single<std::string, size_t,
                                      uint64_t, std::vector<char>,
                                      int64_t>(SQL_DELETE_ANY_LRU, key);
        return std::make_from_tuple<CacheLRU>(raw_entry);
    }
    
};
