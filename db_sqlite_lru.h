#include <cstdint>
#include <memory>
#include <execution>
#include <tuple>
#include <vector>
#include <iostream>
#include <variant>
#include <tuple>
#include <stdexcept>
#include "sqlite_base.h"
#include "sql_statement.h"

class SQLiteLRU : private SQLiteBase {
public:
    struct CacheLRU{
        std::string key;
        size_t size;
        uint64_t download_time;
        std::vector<char> hash;
        int64_t sequence;
        
        CacheLRU(const std::string& key, size_t size, uint64_t download_time,
                 const std::vector<char> &hash, int64_t sequence = 0) : key(key), size(size),
                 download_time(download_time), hash(hash), sequence(sequence) {}
    };

    struct MetaLRU{
        size_t cache_size, max_size;
        int64_t sequence;
        MetaLRU(size_t cache_size, size_t max_size, int64_t sequence) : cache_size(cache_size),
        max_size(max_size), sequence(sequence) {}
    };
    
    const bool is_new;

    SQLiteLRU(const std::string& work_dir, const std::string& db_name) :
              SQLiteBase(work_dir, db_name), is_new(!open()){
        if(is_new){
            execute(SQL_CREATE_LRU);
            execute(SQL_CREATE_METALRU);
        }else{
            execute(SQL_CHECK_LRU);
            execute(SQL_CHECK_METALRU);
        }
        execute(SQL_CREATE_INDEX_LRU);
    }

    int insert_lru(const CacheLRU& entry){ //
        return execute(SQL_INSERT_LRU, entry.key, entry.size, entry.download_time,
                entry.hash, entry.sequence);
    }

    int insert_meta(const MetaLRU& entry){
        return execute(SQL_INSERT_METALRU, entry.cache_size, entry.max_size, entry.sequence);
    }

    int update_lru_seq(const std::string& key, int64_t sequence){
        return execute(SQL_UPDATE_LRU_SEQ, sequence, key);
    }

    int update_lru_content(const std::string& key, size_t size){
        return execute(SQL_UPDATE_LRU_CONTENT, size, key);
    }

    int update_meta(const MetaLRU& entry){
        return execute(SQL_UPDATE_METALRU, entry.cache_size, entry.max_size, entry.sequence);
    }

    int query_lru_count(const std::string& key){
        return SQLiteBase::query_count(SQL_QUERY_COUNT_LRU, key);
    }

    CacheLRU query_lru_single(const std::string& key){
        auto raw_entry = query_single<std::string, size_t,
                                      uint64_t, std::vector<char>,
                                      int64_t>(SQL_QUERY_SINGLE_LRU, key);
        return std::make_from_tuple<CacheLRU>(raw_entry);
    }

    CacheLRU query_lru_old(){
        auto raw_entry = query_single<std::string, size_t,
                                      uint64_t, std::vector<char>,
                                      int64_t>(SQL_QUERY_OLD_LRU);
        return std::make_from_tuple<CacheLRU>(raw_entry);
    }
    
    CacheLRU query_lru_new(){
        auto raw_entry = query_single<std::string, size_t,
                                      uint64_t, std::vector<char>,
                                      int64_t>(SQL_QUERY_NEW_LRU);
        return std::make_from_tuple<CacheLRU>(raw_entry);
    }

    std::vector<CacheLRU> query_lru_all(){
        auto raw_data = query_multi<std::string, size_t, uint64_t,
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
    
    CacheLRU delete_lru_single(const std::string& key){
        auto raw_entry = query_single<std::string, size_t,
                                      uint64_t, std::vector<char>,
                                      int64_t>(SQL_DELETE_SINGLE_LRU, key);
        return std::make_from_tuple<CacheLRU>(raw_entry);
    }
   
    CacheLRU delete_lru_old(){
        auto raw_entry = query_single<std::string, size_t,
                                      uint64_t, std::vector<char>,
                                      int64_t>(SQL_DELETE_OLD_LRU);
        return std::make_from_tuple<CacheLRU>(raw_entry);
    }
    
};
