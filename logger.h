/*
 * Logger (console/rotating file) for x-cache-manager and other application
 * Notice: this logger is not binary-safe but thread-safe
 * The logging level can be changed on the fly
 * It use non-blocking async i/o but may block when the queue is full
 *
 */

#include <atomic>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <queue>
#include <shared_mutex>
#include <stdexcept>
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include <spdlog/async_logger.h>
#include <spdlog/details/registry.h>
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#define DEFAULT_LOG_LEVEL LOG_LEVEL_INFO
#define DEFAULT_BUFF_SIZE 4096
#define DEFAULT_FILE "cache-manager.log"
#define DEFAULT_RT_SIZE 1048576
#define DEFAULT_RT_NUMS 5


class LoggerError : public std::runtime_error{
public:
    explicit LoggerError(const std::string& err) : std::runtime_error(err) {}
};

class Logger{
public:
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
        LOG_LEVEL_OFF,
        LOG_LEVEL_CRITICAL,
        LOG_LEVEL_ERROR,
        LOG_LEVEL_WARN,
        LOG_LEVEL_INFO,
        LOG_LEVEL_DEBUG
    };

    struct MultiLog{
    public:
        MultiLog(Logger* logger) : logger(logger) {}
        void push(const std::string& msg, int level){
            if(level < LOG_LEVEL_CRITICAL || level > LOG_LEVEL_DEBUG)return;
            logs.push(std::make_pair(msg, level)); 
        }

        void pop(){
            if(logs.size())logs.pop();
        }

        void submit(){
            if(logs.empty())return;
            logger->logger_lock.lock();
            while(logs.size()){
                if(logs.front.second > logger->)
                switch (logs.front().second){
                    case LOG_LEVEL_CRITICAL:
                        logger->put_critical(logs.front().first);
                        break;
                    case LOG_LEVEL_ERROR

                    default:
                        break;

                }

                logs.pop(); 
            }
            logger->logger_lock.unlock();
        }
    private:
        Logger* logger;
        std::queue<std::pair<std::string, int>> logs;
    };

    Logger() : stderr_enabled(false), logger_level_console(LOG_LEVEL_OFF),
               logger_level_files(LOG_LEVEL_OFF){}

    ~Logger();

    void set_level(int type, int level);
    void set_stderr(bool enable);

    void setup(int logger, size_t buffer_size = DEFAULT_BUFF_SIZE,
               const std::string& file_name = DEFAULT_FILE,
               size_t rotating_size = DEFAULT_RT_SIZE,
               size_t rotating_nums = DEFAULT_RT_NUMS);

    void put_critical(const std::string& msg);
    void put_error(const std::string& msg);
    void put_warn(const std::string& msg); // always try to put into logger_stderr
    void put_info(const std::string& msg);
    void put_debug(const std::string& msg);

    std::unique_ptr<MultiLog> new_multi(const std::string& msg = "", int level = LOG_LEVEL_INFO);

private:
    //logger status
    std::atomic<bool> stderr_enabled;
    std::atomic<int> logger_level_console, logger_level_files;

    std::shared_mutex logger_lock;

    std::shared_ptr<spdlog::sinks::stdout_color_sink_st> sink_stdout;
    std::shared_ptr<spdlog::sinks::stderr_color_sink_st> sink_stderr;
    std::shared_ptr<spdlog::sinks::rotating_file_sink_st> sink_files;
    
    std::shared_ptr<spdlog::details::thread_pool> pool_stdout, pool_stderr, pool_files;

    std::unique_ptr<spdlog::async_logger> logger_stdout,  logger_stderr, logger_files;

};
