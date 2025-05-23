#include <cstdint>
#include <cstddef>
#include <functional>
#include <iostream>
#include <list>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

class AlgoErrorLRU : public std::runtime_error{
public:
    explicit AlgoErrorLRU(const std::string& err) : std::runtime_error(err) {}
};

class LRU{
public:
    struct Cache{
        std::string key;
        size_t size;
        uint64_t download_time;
        std::vector<char> hash;
    };
    using RemoveCallback = std::function<void(std::vector<Cache>)>;
    using Iter = std::list<Cache>::iterator;
    LRU(size_t size, RemoveCallback cb);
    ~LRU() = default;
    
    void init(std::vector<Cache>&& caches);
    std::vector<Cache> backup();
    bool put(const Cache& cache);
    bool renew(const std::string& key);
    bool update(const std::string& key, size_t size);
    void resize(size_t new_size);
    Cache query(const std::string& key) const;
    void display() const;

private:
    size_t max_size, cache_size;
    RemoveCallback remove_callback;

    std::list<Cache> cache_list;
    std::unordered_map<std::string, std::list<Cache>::iterator> cache_map;

    void remove_cache(size_t required);
    void update_size(Iter it, size_t size);
};

