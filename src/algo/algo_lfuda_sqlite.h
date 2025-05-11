/*
 * This file is the implementation of LFU-DA algorithm (simple version) for
 * x-cache-manager (use sqlite as database and ds provider)
 * Simple dynamic aging policy is applied.
 * If you want to change the sorting method,
 * please check sql_statement.h
 * Complexity O(logN)
 */

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <set>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <utility>
#include <algorithm>

#include "db_sqlite_lfuda.h"

class AlgoErrorLFUDA : public std::runtime_error{
public:
    explicit AlgoErrorLFUDA(const std::string& err) : std::runtime_error(err) {}
};

class LFUDA{
public:
    using Cache = SQLiteLFUDA::CacheLFUDA;
    using Meta = SQLiteLFUDA::MetaLFUDA;
    using RemoveCallback = std::function<void(std::vector<Cache>)>;
    
    LFUDA(std::shared_ptr<SQLiteLFUDA> db, RemoveCallback cb);
    ~LFUDA();
    
    void init();
    void backup() const;
    bool put(const Cache& cache);
    bool renew(const std::string& key, uint64_t timestamp);
    bool update(const std::string& key, size_t new_size);//TODO: add hash support
    void resize(size_t new_size);
    Cache query(const std::string& key) const;
    void display() const;

private:
    mutable std::shared_ptr<SQLiteLFUDA> db_sqlite;
    Meta meta_lfuda;
    RemoveCallback remove_callback;
    
    bool remove_cache(size_t required, const std::string& mark = "");
    void update_size(Cache& it, size_t new_size);
};
