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
    
    void execute(const char* sql);
    
    template<typename... Args>
    void execute(const char* sql, Args&&... args){ // update/insert (delete)
        sqlite3_stmt* stmt = nullptr;    
        sqlite3_pre(sql, &stmt);

        int index = 1;
        (bind_value(stmt, index++, std::forward<Args>(args)), ...);      
        int res = sqlite3_step(stmt);
        sqlite3_final(res, &stmt);
    }

/*
    template<typename... Ts, typename... Args>
    std::vector<std::tuple<Ts...>> query(const char* sql, Args&&... args){
        sqlite3_stmt* stmt = nullptr;
        sqlite3_pre(sql, &stmt);

        int index = 1;
        ((bind_value(stmt, index++, std::forward<Args>(args))), ...);

        int res;
        std::vector<std::tuple<Ts...>> results;
        while((res = sqlite3_step(stmt)) == SQLITE_ROW){
            results.push_back(get_row<Ts...>(stmt));
        }

        sqlite3_final(res, &stmt);
        return results;
    }
*/
    template<typename... Ts>
    std::vector<std::tuple<Ts...>> query(const char* sql){
        sqlite3_stmt* stmt = nullptr;
        sqlite3_pre(sql, &stmt);

        int res;
        std::vector<std::tuple<Ts...>> results;
        while((res = sqlite3_step(stmt)) == SQLITE_ROW){
            results.push_back(get_row<Ts...>(stmt));
        }

        sqlite3_final(res, &stmt);
        return results;
    }


private:
    std::string db_path;
    sqlite3* db;
    
    void sqlite3_pre(const char* sql, sqlite3_stmt** stmt);
    void sqlite3_final(int res, sqlite3_stmt** stmt);

    template<typename T>
    static T get_column_value(sqlite3_stmt* stmt, int col){
        if constexpr(std::is_same_v<T, int>){
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

};

