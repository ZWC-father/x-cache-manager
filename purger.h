#include <iostream>
#include <thread>
#include <queue>
#include <utility>
#include <condition_variable>
#include <string>
#include <atomic>
#include <mutex>
#include <memory>
#include <chrono>
#include <functional>
#include <algorithm>
#include "curl/curl.h"

#define VALID_RESPONSE "Key:"
#define DEFAULT_SERVER "http://127.0.0.1"
#define DEFAULT_CMD "PURGE"
#define QUEUE_MAX 1024
#define THREAD_MAX 4
#define TRY_MAX 3
#define TIMEOUT 1000 //ms

class Purger{
public:
    //TODO: add bind address
    //TODO: make logging system thread safe
    using FailCallback = std::function<void(const std::string, int)>;
    Purger(FailCallback callback, 
           const std::string& server = DEFAULT_SERVER,
           const std::string& command = DEFAULT_CMD,
           size_t queue_max = QUEUE_MAX,
           int thread_max = THREAD_MAX,
           int try_max = TRY_MAX,
           long timeout = TIMEOUT);

    ~Purger();
    
    bool add(const std::string& path);
    size_t size() const;
    void stop();
    
    
private:
    FailCallback fail_callback;
    std::string server_name;
    const std::string purge_command;
    const size_t queue_max;
    const int thread_max, try_max;
    const long timeout;

    std::queue<std::pair<const std::string, int>> purge_queue;
    mutable std::mutex queue_lock;
    std::atomic<bool>stop_signal = 0;
    std::atomic<int>running_worker = 0;
    std::condition_variable worker_cv;

    void worker();

};


