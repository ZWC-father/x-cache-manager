#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <type_traits>
#include <filesystem>
#include <sqlite3.h>

namespace fs = std::filesystem;

template<typename T>
struct always_false : std::false_type {};

class SQLiteError : public std::runtime_error{
public:
    explicit SQLiteError(const std::string& err) : std::runtime_error(err){}
};

class SQLiteBase{
public:
    SQLiteBase(const std::string& work_dir, const std::string& db_name);
    ~SQLiteBase();
    bool open();
    
    int execute(const char* sql);//must be non-query and single-step sql
    int execute_noexcept(const char* sql);

    template<typename... Args>
    int execute(const char* sql, Args&&... args){ // should be non_query sql
        sqlite3_stmt* stmt = nullptr;    
        sqlite3_pre(sql, &stmt);

        int res, index = 1;
        (bind_value(stmt, index++, std::forward<Args>(args)), ...);
        while((res = sqlite3_step(stmt)) == SQLITE_ROW);
        perror(res, &stmt);

        sqlite3_final(&stmt);
        return sqlite3_changes(db);
    }
    
    template<typename... Args>
    int query_count(const char* sql, Args&&... args){
        sqlite3_stmt* stmt = nullptr;
        sqlite3_pre(sql, &stmt);
        
        int res, index = 1, count = 0;
        (bind_value(stmt, index++, std::forward<Args>(args)), ...);
        res = sqlite3_step(stmt);
        perror(res, &stmt);
        
        if(res == SQLITE_ROW)count = sqlite3_column_int(stmt, 0);
        res = sqlite3_step(stmt);
        perror(res, &stmt);
        sqlite3_final(&stmt);
        
        if(res != SQLITE_DONE)throw SQLiteError("too many entries");
        return count;
    }

    template<typename... Ts, typename... Args>
    std::tuple<Ts...> query_single(const char* sql, Args&&... args){
        sqlite3_stmt* stmt = nullptr;
        sqlite3_pre(sql, &stmt);

        int res, index = 1;
        std::tuple<Ts...> result{};
        ((bind_value(stmt, index++, std::forward<Args>(args))), ...);
        res = sqlite3_step(stmt);
        perror(res, &stmt);
        
        if(res == SQLITE_ROW)result = get_row<Ts...>(stmt);
        res = sqlite3_step(stmt);
        perror(res, &stmt);
        sqlite3_final(&stmt);
        
        if(res != SQLITE_DONE)throw SQLiteError("too many entries");
        return result;
    }

    template<typename... Ts, typename... Args>
    std::vector<std::tuple<Ts...>> query_multi(const char* sql, Args&&... args){
        sqlite3_stmt* stmt = nullptr;
        sqlite3_pre(sql, &stmt);

        int res, index = 1, count = 0;
        ((bind_value(stmt, index++, std::forward<Args>(args))), ...);
        
        std::vector<std::tuple<Ts...>> results;
        while((res = sqlite3_step(stmt)) == SQLITE_ROW){
            results.push_back(get_row<Ts...>(stmt));
        }
        perror(res, &stmt);

        sqlite3_final(&stmt);
        return results;
    }
    


private:
    std::string db_path;
    sqlite3* db;
    
    void sqlite3_pre(const char* sql, sqlite3_stmt** stmt);
    void sqlite3_final(sqlite3_stmt** stmt);
    void perror(int res, sqlite3_stmt** stmt);

    template<typename T>
    static T get_column_value(sqlite3_stmt* stmt, int col){
        if constexpr(std::is_null_pointer_v<T>){
            return nullptr;
        }else if constexpr(std::is_same_v<T, int>){
            return sqlite3_column_int(stmt, col);
        }else if constexpr(std::is_same_v<T, int64_t>){
            return sqlite3_column_int64(stmt, col);
        }else if constexpr(std::is_same_v<T, uint64_t>){
            return static_cast<uint64_t>(sqlite3_column_int64(stmt, col));
        }else if constexpr(std::is_same_v<T, double>){
            return sqlite3_column_double(stmt, col);
        }else if constexpr(std::is_same_v<T, std::string>){
            const unsigned char* text = sqlite3_column_text(stmt, col);
            return text ? std::string(reinterpret_cast<const char*>(text)) : std::string{};
        }else if constexpr(std::is_same_v<T, std::vector<char>>){
            const void* data = sqlite3_column_blob(stmt, col);
            int size = sqlite3_column_bytes(stmt, col);
            const char* p = reinterpret_cast<const char*>(data);
            return p ? std::vector<char>(p, p + size) : std::vector<char>{};
            //        }else if constexpr(std::is_same_v<T, std::nullptr_t>>){
//            return sqlite3_col
        }else{
            static_assert(always_false<T>::value, "unsupported type in get_column_value()");
        }
    }

    template<typename... Ts, size_t... Is>
    static std::tuple<Ts...> get_row_impl(sqlite3_stmt* stmt, std::index_sequence<Is...>){
        return std::make_tuple(get_column_value<Ts>(stmt, static_cast<int>(Is))...);
    }

    template<typename... Ts>
    static std::tuple<Ts...> get_row(sqlite3_stmt* stmt){
        return get_row_impl<Ts...>(stmt, std::index_sequence_for<Ts...>{});
    }

    void bind_value(sqlite3_stmt* stmt, int index, int value);
    void bind_value(sqlite3_stmt* stmt, int index, int64_t value);
    void bind_value(sqlite3_stmt* stmt, int index, uint64_t value);
    void bind_value(sqlite3_stmt* stmt, int index, double value);
    void bind_value(sqlite3_stmt* stmt, int index, const std::string& value);
    void bind_value(sqlite3_stmt* stmt, int index, const std::vector<char>& value);
    void bind_value(sqlite3_stmt* stmt, int index, std::nullptr_t value);
//    void bind_value(sqlite3_stmt* stmt, int index, std::nullptr_t value);

};

