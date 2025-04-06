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
    using Meta = SQLiteLRU::MetaLRU;
    using RemoveCallback = std::function<void(std::vector<Cache>)>;
    
    LRU(std::shared_ptr<SQLiteLRU> db, RemoveCallback cb);
    ~LRU();
    
    void init();
    void backup() const;
    bool put(const Cache& cache);
    bool renew(const std::string& key);
    bool update(const std::string& key, size_t size);
    void resize(size_t new_size);
    Cache query(const std::string& key) const;
    void display() const;

private:
    mutable std::shared_ptr<SQLiteLRU> db_sqlite;
    Meta meta_lru;
    RemoveCallback remove_callback;

    bool remove_cache(size_t required, const std::string& mark = "");
    void update_size(Cache& cache, size_t size);
};

