#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <filesystem>
#include <condition_variable>

#include <asio.hpp>
#include <glob.h>

using asio::io_context;

class AsyncDeleter{
public:
    using FailCallback = std::function<void(const std::string& file)>;
    using WorkGuard = asio::executor_work_guard<asio::io_context::executor_type>;
    AsyncDeleter(size_t thr);
    ~AsyncDeleter();
    void submit(const std::string& file);

private:
    const size_t threads;
    asio::io_context io;
    WorkGuard work_guard;
    std::vector<std::thread> thread_pool;
};
