#include <tuple>
#include <vector>
#include <iostream>
#include <variant>
#include <tuple>
#include <stdexcept>
#include "sqlite_base.h"
#include "sql_statement.h"


class SQLiteLFUDA : public SQLiteBase {
    
    struct CacheLFUDA{
        std::string key;
        size_t size;
        uint64_t download_time;
        std::vector<char> hash;
        uint64_t timestamp;
        uint64_t freq;
        uint64_t eff;
    };
    
    SQLiteLFUDA(const std::string& work_dir, const std::string& db_name) :
              SQLiteBase(work_dir, db_name){
        if(open())execute(SQL_CHECK_LFUDA);
        else execute(SQL_CREATE_LFUDA);
    }

    void insert(const CacheLFUDA& value){
        execute(SQL_INSERT_LFUDA, value.key, value.size, value.download_time,
                value.hash, value.timestamp, value.freq, value.eff);
    }

/*
    void update(const std::string& key, uint64_t timestamp, uint64_t freq, uint64_t eff){
        execute(SQL_UPDATE_LFUDA, timestamp, freq, eff);
    }

    void update(const std::string& key, size_t size, const std::vector<char>& hash){
        execute(SQL_UPDATE_LFUDA_CONTENT, size, hash);
    }
*/

    template<typename... Ts>
    std::vector<CacheLFUDA> query_all(){
        std::vector<std::tuple<Ts...>> raw_data = query(SQL_QUERY_LFUDA);
        std::vector<CacheLFUDA> data;
        while(raw_data.size()){
            data.emplace_back(std::make_from_tuple<Ts...>(raw_data.back));
            raw_data.pop_back();
        }
        return data;
    }

    void remove_all(const std::string& key){
        execute(SQL_DELETE_LFUDA);
    }

};
