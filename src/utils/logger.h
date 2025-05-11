/*
 * Logger (console/rotating file) for x-cache-manager and other application
 * The logging level can be changed on the fly
 * It use non-blocking async i/o but may block when the queue is full
 *
 * Warning 1: This logger is not binary-safe but thread-safe
 * Warning 2: If you want to enable stderr, the log with level lower than
 * warn(included) will be writen to stderr.
 * Warning 3: If you enable stderr, and put in multiple logs with different
 * level, it may reordered, as logger_stdout and logger_stderr preserve their
 * own sink & queue
 * Warning 4: If you already setup stdout logging, the buffer_size will be
 * shared with stderr as they use the same thread pool
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <atomic>
#include <cstdint>
#include <exception>
#include <iostream>
#include <mutex>
#include <queue>
#include <cmath>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>
#include <memory>
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include <spdlog/async_logger.h>
#include <spdlog/details/registry.h>
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include "logging_zones.h"

#define DEFAULT_LOG_ZONE LOG_ZONE_MAIN
#define DEFAULT_LOG_LEVEL LOG_LEVEL_INFO
#define DEFAULT_BUFF_SIZE 16384            //16K
#define DEFAULT_FILE "cache-manager.log"
#define DEFAULT_RT_SIZE 5242880            //5M
#define DEFAULT_RT_NUMS 5


class LoggerError : public std::runtime_error{
public:
    explicit LoggerError(const std::string& err) : std::runtime_error(err) {}
};

class Logger{
public:
    static constexpr int TOTAL_LEVELS = 6;

    enum {
        LOG_LOGGER_STDOUT = 1,
        LOG_LOGGER_STDERR = 2,
        LOG_LOGGER_FILES = 4
    };
    
    enum {
        LOG_TYPE_CONSOLE = 1,  //stdout & stderr
        LOG_TYPE_FILES = 2
    };

    enum {
        LOG_LEVEL_OFF = 0,
        LOG_LEVEL_CRITICAL,
        LOG_LEVEL_ERROR,
        LOG_LEVEL_WARN,
        LOG_LEVEL_INFO,
        LOG_LEVEL_DEBUG
    };

    struct MultiLog{
    public:
        MultiLog(Logger* logger, int zone) : logger_ptr(logger),
                                             logging_zone(zone) {}
        void put_critical(const std::string& msg){
            if(!logger_ptr->should_log[LOG_LEVEL_CRITICAL].load(std::memory_order_relaxed))return;
            logs.push(std::make_pair(msg, LOG_LEVEL_CRITICAL));
        }
        
        void put_error(const std::string& msg){
            if(!logger_ptr->should_log[LOG_LEVEL_ERROR].load(std::memory_order_relaxed))return;
            logs.push(std::make_pair(msg, LOG_LEVEL_ERROR));
        }

        void put_warn(const std::string& msg){
            if(!logger_ptr->should_log[LOG_LEVEL_WARN].load(std::memory_order_relaxed))return;
            logs.push(std::make_pair(msg, LOG_LEVEL_WARN));
        }
        
        void put_info(const std::string& msg){
            if(!logger_ptr->should_log[LOG_LEVEL_INFO].load(std::memory_order_relaxed))return;
            logs.push(std::make_pair(msg, LOG_LEVEL_INFO));
        }
        
        void put_debug(const std::string& msg){
            if(!logger_ptr->should_log[LOG_LEVEL_DEBUG].load(std::memory_order_relaxed))return;
            logs.push(std::make_pair(msg, LOG_LEVEL_DEBUG));
        }
        
        void put(const std::string& msg, int level){
            if(level < LOG_LEVEL_CRITICAL || level > LOG_LEVEL_DEBUG)return;
            if(!logger_ptr->should_log[level].load(std::memory_order_relaxed))return;
            logs.push(std::make_pair(msg, level)); 
        }

        void pop(){
            if(logs.size())logs.pop();
        }

        void submit(){
            if(logs.empty())return;

            std::unique_lock<std::shared_mutex> lock(logger_ptr->logger_lock);
            while(logs.size()){
                switch (logs.front().second){
                    case LOG_LEVEL_CRITICAL:
                        logger_ptr->put_critical_impl(logging_prefix[logging_zone] +
                                                      logs.front().first);
                        break;
                    case LOG_LEVEL_ERROR:
                        logger_ptr->put_error_impl(logging_prefix[logging_zone] +
                                                   logs.front().first);
                        break;
                    case LOG_LEVEL_WARN:
                        logger_ptr->put_warn_impl(logging_prefix[logging_zone] +
                                                  logs.front().first);
                        break;
                    case LOG_LEVEL_INFO:
                        logger_ptr->put_info_impl(logging_prefix[logging_zone] +
                                                  logs.front().first);
                        break;
                    case LOG_LEVEL_DEBUG:
                        logger_ptr->put_debug_impl(logging_prefix[logging_zone] +
                                                   logs.front().first);
                        break;
                    default:
                        break;
                }
                logs.pop();
            }
        }
    private:
        Logger* logger_ptr;
        int logging_zone;
        std::queue<std::pair<std::string, int>> logs;
    };

    Logger(){
        logger_level_console.store(LOG_LEVEL_OFF);
        logger_level_files.store(LOG_LEVEL_OFF);
        stderr_enabled.store(false);
        for(int i = 1; i < TOTAL_LEVELS; i++)should_log[i].store(false);
    }


    void setup(int logger, size_t buffer_size = DEFAULT_BUFF_SIZE,
               const std::string& file_name = DEFAULT_FILE,
               size_t rotating_size = DEFAULT_RT_SIZE,
               size_t rotating_nums = DEFAULT_RT_NUMS);
    void set_level(int type, int level);
    void set_stderr(bool enable);

    std::unique_ptr<MultiLog> new_multi(int zone);
    std::unique_ptr<MultiLog> new_multi(int zone, int min_level);
    
    
    void put_critical(int zone, const std::string& msg);
    void put_error(int zone, const std::string& msg);
    void put_warn(int zone, const std::string& msg);
    // always try to put into logger_stderr
    void put_info(int zone, const std::string& msg);
    void put_debug(int zone, const std::string& msg);
    
    template<typename... Args>
    void put_critical(int zone, const Args&... args){
        if(!should_log[LOG_LEVEL_CRITICAL].load(std::memory_order_relaxed))return;
        std::ostringstream oss;
        oss << logging_prefix[zone];
        (oss << ... << args);
        std::shared_lock<std::shared_mutex> lock(logger_lock);
        put_critical_impl(oss.str());
    }
    
    template<typename... Args>
    void put_error(int zone, const Args&... args){
        if(!should_log[LOG_LEVEL_ERROR].load(std::memory_order_relaxed))return;
        std::ostringstream oss;
        oss << logging_prefix[zone];
        (oss << ... << args);
        std::shared_lock<std::shared_mutex> lock(logger_lock);
        put_error_impl(oss.str());
    }
    
    template<typename... Args>
    void put_warn(int zone, const Args&... args){
        if(!should_log[LOG_LEVEL_WARN].load(std::memory_order_relaxed))return;
        std::ostringstream oss;
        oss << logging_prefix[zone];
        (oss << ... << args);
        std::shared_lock<std::shared_mutex> lock(logger_lock);
        put_warn_impl(oss.str());
    }

    template<typename... Args>
    void put_info(int zone, const Args&... args){
        if(!should_log[LOG_LEVEL_INFO].load(std::memory_order_relaxed))return;
        std::ostringstream oss;
        oss << logging_prefix[zone];
        (oss << ... << args);
        std::shared_lock<std::shared_mutex> lock(logger_lock);
        put_info_impl(oss.str());
    }

    template<typename... Args>
    void put_debug(int zone, const Args&... args){
        if(!should_log[LOG_LEVEL_DEBUG].load(std::memory_order_relaxed))return;
        std::ostringstream oss;
        oss << logging_prefix[zone];
        (oss << ... << args);
        std::shared_lock<std::shared_mutex> lock(logger_lock);
        put_debug_impl(oss.str());
    }


private:
    //logger status
    std::atomic<int> logger_level_console, logger_level_files;
    std::atomic<bool> stderr_enabled;
    std::atomic<bool> should_log[TOTAL_LEVELS];

    std::shared_mutex logger_lock;

    std::shared_ptr<spdlog::sinks::stdout_color_sink_st> sink_stdout;
    std::shared_ptr<spdlog::sinks::stderr_color_sink_st> sink_stderr;
    std::shared_ptr<spdlog::sinks::rotating_file_sink_st> sink_files;
    
    std::shared_ptr<spdlog::details::thread_pool> pool_console, pool_files;

    std::shared_ptr<spdlog::async_logger> logger_stdout, logger_stderr, logger_files;
    
    void put_critical_impl(const std::string& msg);
    void put_error_impl(const std::string& msg);
    void put_warn_impl(const std::string& msg);
    void put_info_impl(const std::string& msg);
    void put_debug_impl(const std::string& msg);

};

#endif
