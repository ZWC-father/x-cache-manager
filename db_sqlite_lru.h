#include <tuple>
#include <vector>
#include <iostream>
#include <variant>
#include <tuple>
#include <stdexcept>
#include "sqlite_base.h"
#include "sql_statement.h"

class SQLiteLRU : public SQLiteBase {
    struct CacheLRU{
        std::string key;
        size_t size;
        uint64_t download_time;
        std::vector<char> hash;
        int64_t sequence;
    };
    
    SQLiteLRU(const std::string& work_dir, const std::string& db_name) :
              SQLiteBase(work_dir, db_name){
        if(open())execute(SQL_CHECK_LRU);
        else execute(SQL_CREATE_LRU);
        execute(SQL_CREATE_INDEX_LRU);
    }
    
    int insert(const CacheLRU& entry){ //
        return execute(SQL_INSERT_LRU, entry.key, entry.size, entry.download_time,
                entry.hash, entry.sequence);
    }

    int update_content(const std::string& key, size_t size){
        return execute(SQL_UPDATE_LRU_CONTENT, key, size);
    }

    int count(const std::string& key){
        return query_num(SQL_QUERY_NUM_LRU, key);
    }

    template<typename... Ts>
    std::vector<CacheLRU> query_all(){
        std::vector<std::tuple<Ts...>> raw_data = query(SQL_QUERY_ALL_LRU);
        std::vector<CacheLRU> data;//add memory optimization
        while(raw_data.size()){
            data.emplace_back(std::make_from_tuple<Ts...>(raw_data.back));
            raw_data.pop_back();
        }
        return data;
    }
    
    template<typename... Ts>
    CacheLRU delete_old(){
        std::tuple<Ts...> raw_entry = query_single(SQL_DELETE_OLD_LRU);
        return std::make_from_tuple<Ts...>(raw_entry);
    }
    
    template<typename... Ts>
    CacheLRU delete_any(const std::string& key){
        std::tuple<Ts...> raw_entry = query_single(SQL_DELETE_ANY_LRU, key);
        return std::make_from_tuple<Ts...>(raw_entry);
    }
    
};
