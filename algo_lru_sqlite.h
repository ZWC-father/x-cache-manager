#include <cstdint>
#include <cstddef>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

#include "db_sqlite_lru.h"

class AlgoErrorLRU : public std::runtime_error{
public:
    explicit AlgoErrorLRU(const std::string& err) : std::runtime_error(err) {}
};

class LRU{
public:
    using Cache = SQLiteLRU::CacheLRU;
    using RemoveCallback = std::function<void(std::vector<Cache>)>;
    using Iter = std::list<Cache>::iterator;
    LRU(std::shared_ptr<SQLiteLRU> db, RemoveCallback cb);
    ~LRU() = default;
    
    void init();
    void backup();
    bool put(const Cache& cache);
    bool renew(const std::string& key);
    bool update_content(const std::string& key, size_t size);
    void resize(size_t new_size);
    Cache query(const std::string& key) const;
    void display() const;

private:
    SQLiteLRU::MetaLRU meta_lru;
    std::shared_ptr<SQLiteLRU> db_sqlite;
    RemoveCallback remove_callback;

    void remove_cache(size_t required);
    void update_size(Cache& cache, size_t size);
};

