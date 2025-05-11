/*
 * This is the implementation of LFU-DA algorithm (simple version) for x-cache-manager
 * Simple dynamic aging policy is applied.
 * Complexity O(logN)
 */

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <set>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <utility>
#include <algorithm>

class AlgoErrorLFUDA : public std::runtime_error{
public:
    explicit AlgoErrorLFUDA(const std::string& err) : std::runtime_error(err) {}
};

class LFUDA{
public:
    struct Cache{
        std::string key;
        size_t size;
        uint64_t download_time;
        std::vector<char> hash;
        uint64_t timestamp;
        uint64_t freq;
        uint64_t eff;
        
        bool operator<(const Cache& other) const{
            if(eff != other.eff)return eff < other.eff;
            if(timestamp != other.timestamp)return timestamp < other.timestamp;
            if(size != other.size)return size < other.size;
            return key < other.key;
        }
    };
    using Iter = std::set<Cache>::iterator;
    using RemoveCallback = std::function<void(std::vector<Cache>)>;
    
    LFUDA(size_t size, RemoveCallback cb);
    ~LFUDA() = default;
    void init(std::vector<Cache>&& caches, uint64_t age);
    std::pair<std::vector<Cache>, uint64_t> backup();
    bool put(const Cache& cache);
    bool renew(const std::string& key, uint64_t timestamp);
    bool update(const std::string& key, size_t size);
    void resize(size_t new_size);
    void display() const;
    Cache query(const std::string& key) const;

private:
    size_t max_size, cache_size;
    uint64_t aging;
    RemoveCallback remove_callback;

    std::set<Cache> cache_bst;
    std::unordered_map<std::string, std::set<Cache>::iterator> cache_map;
    
    void remove_cache(size_t required);
    Iter update_size(Iter it, size_t size);
};
