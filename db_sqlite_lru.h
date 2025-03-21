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
        int sequence;
    };
    
    SQLiteLRU(const std::string& work_dir, const std::string& db_name) :
              SQLiteBase(work_dir, db_name){
        if(open())execute(SQL_CHECK_LRU);
        else execute(SQL_CREATE_LRU);
    }
    
    void insert(const CacheLRU& value){
        execute(SQL_INSERT_LRU, value.key, value.size, value.download_time,
                value.hash, value.sequence);
    }

/*
    void update(const std::string& key, int sequence){
        execute(SQL_UPDATE_LRU, sequence);
    }

    void update(const std::string& key, size_t size, const std::vector<char>& hash){
        execute(SQL_UPDATE_LRU_CONTENT, size, hash);
    }
*/
    template<typename... Ts>
    std::vector<CacheLRU> query_all(){
        std::vector<std::tuple<Ts...>> raw_data = query(SQL_QUERY_LRU);
        std::vector<CacheLRU> data;//add optimization for sequence (prevent sorting)
        while(raw_data.size()){
            data.emplace_back(std::make_from_tuple<Ts...>(raw_data.back));
            raw_data.pop_back();
        }
        return data;
    }

    void remove_all(){
        execute(SQL_DELETE_LRU);
    }

};
