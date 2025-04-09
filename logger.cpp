#include "logger.h"
#include <memory>
#include <shared_mutex>

Logger::~Logger(){
    if(logger_files != nullptr)logger_files->flush();
    spdlog::shutdown();
}

void Logger::setup(int logger, size_t buffer_size, const std::string& file_name,
                        size_t rotating_size, size_t rotating_nums){
    try{
        if(logger & LOG_LOGGER_STDOUT){
            if(logger_stdout == nullptr){
                if(sink_stdout == nullptr){
                    sink_stdout = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
                    sink_stdout->set_level(spdlog::level::debug);
                    sink_stdout->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
                    sink_stdout->set_color(spdlog::level::debug, "\033[32m");
                    sink_stdout->set_color(spdlog::level::info,  "\033[37m");
                    sink_stdout->set_color(spdlog::level::warn,  "\033[33m");
                    sink_stdout->set_color(spdlog::level::err,   "\033[31m");
                    sink_stdout->set_color(spdlog::level::critical, "\033[35m");
                }

                if(pool_console == nullptr){
                    pool_console = std::make_shared<spdlog::details::thread_pool>(buffer_size, 1);
                }

                logger_stdout = std::make_shared<spdlog::async_logger>("STDOUT",
                sink_stdout, pool_console, spdlog::async_overflow_policy::block);
                
                if(logger_stdout != nullptr){
                    logger_stdout->disable_backtrace();
                    spdlog::register_logger(logger_stdout);
                    logger_level_console.store(DEFAULT_LOG_LEVEL, std::memory_order_release);
                }
            }
        }

        if(logger & LOG_LOGGER_STDERR){
            if(logger_stderr == nullptr){
                if(sink_stderr == nullptr){
                    sink_stderr = std::make_shared<spdlog::sinks::stderr_color_sink_st>();
                    sink_stderr->set_level(spdlog::level::warn);
                    sink_stderr->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
                    sink_stderr->set_color(spdlog::level::warn,  "\033[33m");
                    sink_stderr->set_color(spdlog::level::err,   "\033[31m");
                    sink_stderr->set_color(spdlog::level::critical, "\033[35m");
                }

                if(pool_console == nullptr){
                    pool_console = std::make_shared<spdlog::details::thread_pool>(buffer_size, 1);
                }
                    
                logger_stderr = std::make_shared<spdlog::async_logger>("STDERR",
                sink_stderr, pool_console, spdlog::async_overflow_policy::block); 
            
                if(logger_stderr != nullptr){
                    logger_stderr->disable_backtrace();
                    spdlog::register_logger(logger_stderr);
                    stderr_enabled.store(true, std::memory_order_release);
                }
            }
        }

        if(logger & LOG_LOGGER_FILES){
            if(logger_files == nullptr){
                if(sink_files == nullptr){
                    sink_files = std::make_shared<spdlog::sinks::rotating_file_sink_st>
                                 (file_name, rotating_size, rotating_nums);
                    sink_files->set_level(spdlog::level::debug);
                    sink_files->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
                }
                 
                if(pool_files == nullptr){
                    pool_files = std::make_shared<spdlog::details::thread_pool>(buffer_size, 1);
                }
                    
                logger_files = std::make_shared<spdlog::async_logger>("FILES",
                sink_files, pool_files, spdlog::async_overflow_policy::block);
                
                if(logger_files != nullptr){
                    logger_files->disable_backtrace();
                    spdlog::register_logger(logger_files);
                    logger_level_files.store(DEFAULT_LOG_LEVEL, std::memory_order_release);
                }
            }
        }

    }catch(const std::exception& e){
        throw LoggerError(std::string("[*** LOG SETUP ERROR ***] ") + e.what());
    }

}

void Logger::set_level(int type, int level){
    if(type & LOG_TYPE_CONSOLE){
        if(level >= LOG_LEVEL_OFF && level <= LOG_LEVEL_DEBUG){
            logger_level_console.store(level, std::memory_order_release);
        }
    }

    if(type & LOG_TYPE_FILES){
        if(level >= LOG_LEVEL_OFF && level <= LOG_LEVEL_DEBUG){
            logger_level_files.store(level, std::memory_order_release);
        }
    }
}

void Logger::set_stderr(bool enable){
    stderr_enabled.store(enable, std::memory_order_release);
}

std::unique_ptr<Logger::MultiLog> Logger::new_multi(){
    std::unique_ptr<MultiLog> multi_log = std::make_unique<MultiLog>(this);
    return multi_log;
}

void Logger::put_critical(const std::string& msg){
    std::shared_lock<std::shared_mutex> lock(logger_lock);
    put_critical_impl(msg);
}

void Logger::put_error(const std::string& msg){
    std::shared_lock<std::shared_mutex> lock(logger_lock);
    put_error_impl(msg);
}

void Logger::put_warn(const std::string& msg){
    std::shared_lock<std::shared_mutex> lock(logger_lock);
    put_warn_impl(msg);
}

void Logger::put_info(const std::string& msg){
    std::shared_lock<std::shared_mutex> lock(logger_lock);
    put_info_impl(msg);
}

void Logger::put_debug(const std::string& msg){
    std::shared_lock<std::shared_mutex> lock(logger_lock);
    put_debug_impl(msg);
}

void Logger::put_critical_impl(const std::string& msg){
    if(logger_level_console.load(std::memory_order_relaxed) >= LOG_LEVEL_CRITICAL){
        if(stderr_enabled.load(std::memory_order_relaxed)){
            logger_stderr->error(msg);
            logger_stderr->flush();
        }else{
            logger_stdout->error(msg);
            logger_stdout->flush();
            
        }
    }
    
    if(logger_level_files.load(std::memory_order_relaxed) >= LOG_LEVEL_CRITICAL){
        logger_files->error(msg);
    }
}

void Logger::put_error_impl(const std::string& msg){
    if(logger_level_console.load(std::memory_order_relaxed) >= LOG_LEVEL_ERROR){
        if(stderr_enabled.load(std::memory_order_relaxed)){
            logger_stderr->error(msg);
            logger_stderr->flush();
        }else{
            logger_stdout->error(msg);
            logger_stdout->flush();
        }
    }
    
    if(logger_level_files.load(std::memory_order_relaxed) >= LOG_LEVEL_ERROR){
        logger_files->error(msg);
    }
}

void Logger::put_warn_impl(const std::string& msg){
    if(logger_level_console.load(std::memory_order_relaxed) >= LOG_LEVEL_WARN){
        if(stderr_enabled.load(std::memory_order_relaxed)){
            logger_stderr->warn(msg);
            logger_stderr->flush();
        }else{
            logger_stdout->warn(msg);
            logger_stdout->flush();
        }
    }
    
    if(logger_level_files.load(std::memory_order_relaxed) >= LOG_LEVEL_WARN){
        logger_files->warn(msg);
    }
}

void Logger::put_info_impl(const std::string& msg){
    if(logger_level_console.load(std::memory_order_relaxed) >= LOG_LEVEL_INFO){
        logger_stdout->info(msg);
        logger_stdout->flush();
    }
    
    if(logger_level_files.load(std::memory_order_relaxed) >= LOG_LEVEL_INFO){
        logger_files->info(msg);
    }
}

void Logger::put_debug_impl(const std::string& msg){
    if(logger_level_console.load(std::memory_order_relaxed) == LOG_LEVEL_DEBUG){
        logger_stdout->debug(msg);
        logger_stdout->flush();
    }
    
    if(logger_level_files.load(std::memory_order_relaxed) == LOG_LEVEL_DEBUG){
        logger_files->debug(msg);
    }
}

