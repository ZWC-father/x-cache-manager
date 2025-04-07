#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <list>
#include <stdexcept>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

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
        LOG_ACTION_INIT,
        LOG_ACTION_ENABLE,
        LOG_ACTION_DISABLE
    };
    
    enum {
        LOG_OPT_STRING_MAX,
        LOG_OPT_QUEUE_MAX,
        LOG_OPT_HOLDER_TIMEOUT,
    };

    enum LOG_SINK_ {
        LOG_SINK_STDOUT = 1,
        LOG_SINK_STDERR = 2,
        LOG_SINK_FILES = 4
    };

    Logger() : logger_mode(0), holder_now(0),holder_count(0),
    param{DEFAULT_STRING_MAX, DEFAULT_QUEUE_MAX,
    DEFAULT_HOLDER_TIMEOUT} {}

    ~Logger();

    template<int Opt, typename T>
    void set_option(T arg){
        if(logger_mode)return; //you should not set option after init
        if constexpr(Opt == 0){
            param.string_max = arg;
        }else if constexpr(Opt == 1){
            param.queue_max = arg;
        }else if constexpr(Opt == 2){
            param.holder_timeout = arg;
        }else{
            static_assert(true, "Invalid Option in set_option()");
        }
    }
    
    void set_logger(int type, int action, const std::string& file = DEFAULT_FILE,
                    size_t rotating_size = DEFAULT_RT_SIZE, size_t rotating_num = DEFAULT_RT_NUMS);

    static void put_debug();
    static void put_info();
    static void put_warn();
    static void put_errr();
    static void put_fatal();

private:
    //logger status
    std::atomic<int> logger_mode;
    std::atomic<int> holder_now;
    std::atomic<int> holder_count;

    //params
    struct Param{
        std::atomic<size_t> string_max;
        std::atomic<size_t> queue_max;
        std::atomic<uint32_t> holder_timeout; //ms
    }param;

    struct Holder{
        int holder_id;
        std::chrono::steady_clock::time_point start_time;
    };

    struct Log{
        std::string str; //not binary-safe!
        std::optional<Holder> holder;
    };

    std::list<Log> logs; 
  
    std::shared_ptr<spdlog::sinks::stdout_color_sink_st> sink_stdout;
    std::shared_ptr<spdlog::sinks::stderr_color_sink_st> sink_stderr;
    std::shared_ptr<spdlog::sinks::rotating_file_sink_st> sink_files;

    std::unique_ptr<spdlog::logger> logger_stdout;
    std::unique_ptr<spdlog::logger> logger_stderr;
    std::unique_ptr<spdlog::logger> logger_files;

};
