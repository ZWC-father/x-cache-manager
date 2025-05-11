#include <iostream>
#include <system_error>
#include <thread>
#include <mutex>
#include <chrono>
#include <fstream>
#include <utility>
#include <atomic>
#include <queue>
#include <filesystem>
#include <condition_variable>

#include <asio.hpp>
#include <glob.h>

using asio::io_context;
namespace fs = std::filesystem;

class AsyncDeleter{
public:
    using FailCallback = void(*)(const std::string& file,
                                 std::error_code ec) noexcept;
    //must be noexcept, so not std::function
    using WorkGuard = asio::executor_work_guard<asio::io_context::executor_type>;
    AsyncDeleter(size_t thr, FailCallback cb);
    ~AsyncDeleter();
    void run();
    void stop();
    void force_stop();
    void submit(const std::string& file);
    std::pair<size_t, size_t> status() const;
    

private:
    const size_t threads;
    std::mutex cv_lock;
    std::condition_variable cv;
    std::atomic<size_t> running_threads;
    std::atomic<size_t> working_jobs;
    FailCallback fail_callback;
    asio::io_context io;
    WorkGuard* work_guard;
    std::vector<std::thread> thread_pool;

    void wait_thread();
};
