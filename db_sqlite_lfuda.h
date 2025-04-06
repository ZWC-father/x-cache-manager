#include <algorithm>
#include <cstdint>
#include <execution>
#include <sys/types.h>
#include <tuple>
#include <vector>
#include <iostream>
#include <variant>
#include <tuple>
#include <stdexcept>
#include "sqlite_base.h"
#include "sql_statement.h"


class SQLiteLFUDA : private SQLiteBase {
public:    
    struct CacheLFUDA{
        std::string key;
        size_t size;
        uint64_t download_time;
        std::vector<char> hash;
        uint64_t timestamp;
        uint64_t freq;
        uint64_t eff;
        
        CacheLFUDA(const std::string& key, size_t size, uint64_t download_time,
                   const std::vector<char>& hash, uint64_t timestamp = 0,
                   uint64_t freq = 0, uint64_t eff = 0) : key(key), size(size),
                   download_time(download_time), hash(hash), timestamp(timestamp),
                   freq(freq), eff(eff) {}
    };

    struct MetaLFUDA{
        size_t cache_size, max_size;
        uint64_t global_aging;
        MetaLFUDA(size_t cache_size, size_t max_size, uint64_t global_aging) :
        cache_size(cache_size), max_size(max_size), global_aging(global_aging) {}
    };
    
    struct TFE{
        std::string key;
        uint64_t timestamp;
        uint64_t freq;
        uint64_t eff;
    };

    const bool is_new;

    SQLiteLFUDA(const std::string& work_dir, const std::string& db_name) :
                SQLiteBase(work_dir, db_name), is_new(!open()){
        if(is_new){
            execute(SQL_CREATE_LFUDA);
            execute(SQL_CREATE_METALFUDA);
        }else{
            execute(SQL_CHECK_LFUDA);
            execute(SQL_CHECK_METALFUDA);
        }
        execute(SQL_CREATE_INDEX_LFUDA);
    }

    int insert_lru(const CacheLFUDA& entry){ //
        return execute(SQL_INSERT_LFUDA, entry.key, entry.size, entry.download_time,
               entry.hash, entry.timestamp, entry.freq, entry.eff);
    }

    int insert_meta(const MetaLFUDA& entry){
        return execute(SQL_INSERT_METALFUDA, entry.cache_size, entry.max_size,
               entry.global_aging);
    }

    int update_lfuda_tfe(const std::string& key, uint64_t timestamp,
                         uint64_t freq, uint64_t eff){
        return execute(SQL_UPDATE_TFE_LFUDA, timestamp, freq, eff, key);
    }

    int update_lfuda_content(const std::string& key, size_t size){
        return execute(SQL_UPDATE_CONTENT_LFUDA, size, key);
    }

    int update_meta(const MetaLFUDA& entry){
        return execute(SQL_UPDATE_METALRU, entry.cache_size, entry.max_size,
               entry.global_aging);
    }

    int query_lfuda_count(const std::string& key){
        return SQLiteBase::query_count(SQL_QUERY_COUNT_LFUDA, key);
    }

    CacheLFUDA query_lfuda_single(const std::string& key){
        auto raw_entry = query_single<std::string, size_t,
                                      uint64_t, std::vector<char>,
                                      uint64_t, uint64_t,
                                      uint64_t>(SQL_QUERY_SINGLE_LFUDA, key);
        return std::make_from_tuple<CacheLFUDA>(raw_entry);
    }

    TFE query_lfuda_time(const std::string& key){
        auto entry = query_single<std::string, uint64_t, uint64_t,
                                  uint64_t>(SQL_QUERY_TFE_LFUDA, key);
        return std::make_from_tuple<TFE>(entry);
    }

    CacheLFUDA query_lfuda_worst(){
        auto raw_entry = query_single<std::string, size_t,
                                      uint64_t, std::vector<char>,
                                      uint64_t, uint64_t,
                                      uint64_t>(SQL_QUERY_WORST_LFUDA);
        return std::make_from_tuple<CacheLFUDA>(raw_entry);
    }

    std::vector<CacheLFUDA> query_lfuda_all(){
        auto raw_data = query_multi<std::string, size_t, uint64_t,
                                    std::vector<char>, uint64_t,
                                    uint64_t, uint64_t>(SQL_QUERY_ALL_LFUDA);
        std::vector<CacheLFUDA> data;//TODO: memory optimization
        while(raw_data.size()){
            data.push_back(std::make_from_tuple<CacheLFUDA>(raw_data.back()));
            raw_data.pop_back();
        }
        return data;
    }

    MetaLFUDA query_meta(){
        auto raw_entry = query_single<size_t, size_t,
                                      uint64_t>(SQL_QUERY_METALFUDA);
        return std::make_from_tuple<MetaLFUDA>(raw_entry);
    }
    
    CacheLFUDA delete_lfuda_single(const std::string& key){
        auto raw_entry = query_single<std::string, size_t,
                                      uint64_t, std::vector<char>,
                                      uint64_t, uint64_t,
                                      uint64_t>(SQL_DELETE_SINGLE_LFUDA, key);
        return std::make_from_tuple<CacheLFUDA>(raw_entry);
    }
   
    CacheLFUDA delete_lfuda_old(){
        auto raw_entry = query_single<std::string, size_t,
                                      uint64_t, std::vector<char>,
                                      uint64_t, uint64_t,
                                      uint64_t>(SQL_DELETE_WORST_LFUDA);
        return std::make_from_tuple<CacheLFUDA>(raw_entry);
    }
    

};
