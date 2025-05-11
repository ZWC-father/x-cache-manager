#include "async_deleter.h"
#include <filesystem>

AsyncDeleter::AsyncDeleter(size_t thr, FailCallback cb) : threads(thr),
fail_callback(cb), running_threads(0), working_jobs(0), work_guard(nullptr) {}

AsyncDeleter::~AsyncDeleter(){
    force_stop();
    wait_thread();
    if(work_guard != nullptr)delete work_guard;
}

void AsyncDeleter::wait_thread(){
    if(running_threads.load())std::cerr << "warning: some thread still running, waiting..." << std::endl;
    for(auto &it : thread_pool){
        if(it.joinable())it.join();
    }
}

void AsyncDeleter::run(){
    if(running_threads)throw std::runtime_error("some thread still running");
    
    if(thread_pool.size())io.reset();
    if(work_guard != nullptr){
        delete work_guard;
        work_guard = nullptr;
    }
    work_guard = new WorkGuard(asio::make_work_guard(io));
    
    thread_pool.clear();
    for(size_t i = 0; i < threads; i++){
        thread_pool.emplace_back([this, i]{
            running_threads.fetch_add(1);
            std::cerr << "thread # " << i << " started" << std::endl;//fix logging thread safety
            io.run();
            running_threads.fetch_sub(1);
        });
    }
}

void AsyncDeleter::force_stop(){
    work_guard->reset();
    size_t jobs = working_jobs;
    if(jobs)std::cerr << "warning: " << jobs << " working jobs, force stop" << std::endl;
    io.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    wait_thread();
}

void AsyncDeleter::stop(){
    work_guard->reset();
    std::unique_lock<std::mutex> mtx(cv_lock);
    if(working_jobs.load())cv.wait(mtx,
                           [this](){return working_jobs.load() == 0;});
    io.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    wait_thread();
}

void AsyncDeleter::submit(const std::string& file){
    working_jobs.fetch_add(1);
    asio::post(io, [this, file](){
            std::error_code ec;
            try{
                if(!fs::remove_all(file, ec)){
                    fail_callback(file, ec);
                }
            }catch(const std::exception& e){
                std::cerr << "exception: " << e.what() << std::endl;
            }
            
            if(working_jobs.fetch_sub(1) <= 2){
                std::unique_lock<std::mutex> mtx(cv_lock);
                cv.notify_all();
            }
    });
}

std::pair<size_t, size_t> AsyncDeleter::status() const{
    return std::make_pair(running_threads.load(), working_jobs.load());
}
