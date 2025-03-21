#include "async_deleter.h"
#include <asio/error_code.hpp>
#include <asio/executor_work_guard.hpp>
#include <stdexcept>

AsyncDeleter::AsyncDeleter(size_t thr) : threads(thr),
work_guard(asio::make_work_guard(io)){
    for(size_t i = 0; i < threads; i++){
        thread_pool.emplace_back([this]{
            asio::error_code err;
            io.run(&err);
            if(err)throw std::runtime_error("io_context failed to run");
        });
    }
}

