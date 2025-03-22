#include "sqlite_base.h"
#include <filesystem>

SQLiteBase::SQLiteBase(const std::string& work_dir, const std::string& db_name){
    if(!fs::exists(work_dir) || !fs::is_directory(work_dir)){
        throw SQLiteError("no such directory: " + work_dir);
    }
    
    db_path = work_dir.back() == '/' ? work_dir : work_dir + '/';
    db_path += db_name;
}

SQLiteBase::~SQLiteBase(){
    if(sqlite3_close(db) != SQLITE_OK)std::cerr << "failed to close database" << std::endl;
}

bool SQLiteBase::open(){
    bool flag = fs::exists(db_path);
    if(sqlite3_open(db_path.c_str(), &db) != SQLITE_OK){
        throw SQLiteError("failed to open database: " + std::string(sqlite3_errmsg(db)));
    }
    return flag;
}


void SQLiteBase::sqlite3_pre(const char* sql, sqlite3_stmt** stmt){
    if(sqlite3_prepare_v2(db, sql, -1, stmt, nullptr) != SQLITE_OK){
        throw SQLiteError("sql prepare failed: " + std::string(sqlite3_errmsg(db)));
    }
}

void SQLiteBase::sqlite3_final(sqlite3_stmt** stmt){
    if(sqlite3_finalize(*stmt) != SQLITE_OK){
        throw SQLiteError("sql finalize failed: " + std::string(sqlite3_errmsg(db)));
    }
}

int SQLiteBase::execute(const char *sql) {
    char *err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK){
        std::string error = err ? err : "unknown error";
        sqlite3_free(err);
        throw SQLiteError("simple sql error: " + error);
    }
    return sqlite3_changes(db);
}

/*------------- Bind Value -------------*/
void SQLiteBase::bind_value(sqlite3_stmt* stmt, int index, int value){
    sqlite3_bind_int(stmt, index, value);
}

void SQLiteBase::bind_value(sqlite3_stmt* stmt, int index, int64_t value){
    sqlite3_bind_int64(stmt, index, value);
}

void SQLiteBase::bind_value(sqlite3_stmt* stmt, int index, uint64_t value){
    sqlite3_bind_int64(stmt, index, static_cast<sqlite3_int64>(value));
}

void SQLiteBase::bind_value(sqlite3_stmt* stmt, int index, double value){
    sqlite3_bind_double(stmt, index, value);
}

void SQLiteBase::bind_value(sqlite3_stmt* stmt, int index, const std::string& value){
    sqlite3_bind_text(stmt, index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT);
}

void SQLiteBase::bind_value(sqlite3_stmt* stmt, int index, const std::vector<char>& value){
    sqlite3_bind_blob(stmt, index, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT);
}

void SQLiteBase::bind_value(sqlite3_stmt* stmt, int index, std::nullptr_t value){
    sqlite3_bind_null(stmt, index);
}

/*
void SQLiteBase::bind_value(sqlite3_stmt* stmt, int index, std::nullptr_t value){
    sqlite3_bind_null(stmt, index);
}
*/
/*----------- End Bind Value -----------*/

void SQLiteBase::perror(int res, sqlite3_stmt** stmt){
    if(res != SQLITE_OK && res != SQLITE_ROW && res != SQLITE_DONE){
        sqlite3_finalize(*stmt);
        throw SQLiteError("sql execute error: " + std::string(sqlite3_errmsg(db)));
    }
}

