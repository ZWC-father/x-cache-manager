#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <list>
#include <stdexcept>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#define DEFAULT_LOG_LEVEL LOG_LEVEL_INFO

#define DEFAULT_FILE "cache-manager.log"
#define DEFAULT_RT_SIZE 1048576
#define DEFAULT_RT_NUMS 5

#define DEFAULT_STRING_MAX 0
#define DEFAULT_QUEUE_MAX 1024
#define DEFAULT_HOLDER_TIMEOUT 1000

class LoggerError : public std::runtime_error{
public:
    explicit LoggerError(const std::string& err) : std::runtime_error(err) {}
};

class Logger{
public:
    enum {
        LOG_ACTION_INIT = 1,
        LOG_ACTION_ENABLE = 2,
        LOG_ACTION_DISABLE = 4
    };
    
    enum {
        LOG_OPT_STRING_MAX,
        LOG_OPT_QUEUE_MAX,
        LOG_OPT_HOLDER_TIMEOUT,
        LOG_OPT_LEVEL
    };

    enum {
        LOG_LEVEL_CRITICAL,
        LOG_LEVEL_ERR,
        LOG_LEVEL_WARN,
        LOG_LEVEL_INFO,
        LOG_LEVEL_DEBUG
    };

    enum {
        LOG_MODE_STDOUT = 1,
        LOG_MODE_STDERR = 2,
        LOG_MODE_FILES = 4
    };

    Logger() : logger_mode(0), holder_now(0),holder_count(0),
    param{DEFAULT_STRING_MAX, DEFAULT_QUEUE_MAX,
    DEFAULT_HOLDER_TIMEOUT} {}

    ~Logger();

    template<typename T>
    void set_option(int opt, T arg){
        if(opt == LOG_OPT_STRING_MAX){
            param.string_max.store(arg, std::memory_order_release);
        }else if(opt == LOG_OPT_QUEUE_MAX){
            param.list_max.store(arg, std::memory_order_release);
        }else if(opt == LOG_OPT_HOLDER_TIMEOUT){
            param.holder_timeout(arg, std::memory_order_release);
        }else if(opt == LOG_OPT_LEVEL){
            param.logger_level.store(arg, std::memory_order_release);
        }
    }
    
    void set_logger(int type, int action, const std::string& file = DEFAULT_FILE,
                    size_t rotating_size = DEFAULT_RT_SIZE, size_t rotating_num = DEFAULT_RT_NUMS);

    int hold();
    void release(int holder);
    int put_critical(const std::string& str, int hold = 0);
    int put_err(const std::string& str, int hold = 0);
    int put_warn(const std::string& str, int hold = 0);
    int put_info(const std::string& str, int hold = 0);
    int put_debug(const std::string& str, int hold = 0);

private:
    //logger status
    std::atomic<int> logger_mode;

    //params
    struct Param{
        std::atomic<size_t> string_max;
        std::atomic<size_t> list_max;
        std::atomic<uint32_t> holder_timeout; //ms
        std::atomic<int> logger_level; 
    }param;

    struct Holder{
        int holder_id;
        std::chrono::steady_clock::time_point start_time;
    };

    struct Log{
        std::string str; //not binary-safe!
        std::optional<Holder> holder;
    };

    int holder_now, holder_count;
    std::list<Log> logs;
    std::mutex list_lock;
    std::condition_variable cv;
  
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> sink_stdout;
    std::shared_ptr<spdlog::sinks::stderr_color_sink_mt> sink_stderr;
    std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> sink_files;

    std::unique_ptr<spdlog::logger> logger_stdout;
    std::unique_ptr<spdlog::logger> logger_stderr;
    std::unique_ptr<spdlog::logger> logger_files;

};
