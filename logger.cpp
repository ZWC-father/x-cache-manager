#include "logger.h"
#include <atomic>
#include <exception>
#include <memory>
#include <spdlog/async_logger.h>
#include <spdlog/details/registry.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>


void Logger::setup(int logger, size_t buffer_size, const std::string& file_name,
                        size_t rotating_size, size_t rotating_nums){
    try{
        if(logger & LOG_LOGGER_STDOUT){
            if(logger_stdout == nullptr){
                if(sink_stdout == nullptr){
                    sink_stdout = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
                    sink_stdout->set_color(spdlog::level::debug, "\033[32m");
                    sink_stdout->set_color(spdlog::level::info,  "\033[37m");
                    sink_stdout->set_color(spdlog::level::warn,  "\033[33m");
                    sink_stdout->set_color(spdlog::level::err,   "\033[31m");
                    sink_stdout->set_color(spdlog::level::critical, "\033[35m");
                }

                if(pool_stdout == nullptr){
                    pool_stdout = std::make_shared<spdlog::details::thread_pool>(buffer_size, 1);
                }

                logger_stdout = std::make_unique<spdlog::async_logger>("logger_stdout",
                sink_stdout, pool_stdout, spdlog::async_overflow_policy::block);
                
                if(logger_stdout != nullptr){
                    logger_level_console.store(DEFAULT_LOG_LEVEL, std::memory_order_release);
                }
            }
        }

        if(logger & LOG_LOGGER_STDERR){
            if(logger_stderr == nullptr){
                if(sink_stderr == nullptr){
                    sink_stderr = std::make_shared<spdlog::sinks::stderr_color_sink_st>();
                    sink_stderr->set_color(spdlog::level::warn,  "\033[33m");
                    sink_stderr->set_color(spdlog::level::err,   "\033[31m");
                    sink_stderr->set_color(spdlog::level::critical, "\033[35m");
                }

                if(pool_stderr == nullptr){
                    pool_stderr = std::make_shared<spdlog::details::thread_pool>(buffer_size, 1);
                }
                    
                logger_stderr = std::make_unique<spdlog::async_logger>("logger_stderr",
                sink_stderr, pool_stderr, spdlog::async_overflow_policy::block); 
            
                if(logger_stderr != nullptr){
                    stderr_enabled.store(true, std::memory_order_release);
                }
            }
        }

        if(logger & LOG_LOGGER_FILES){
            if(logger_files == nullptr){
                if(sink_files == nullptr){
                    sink_files = std::make_shared<spdlog::sinks::rotating_file_sink_st>
                                 (file_name, rotating_size, rotating_nums);
                }
                 
                if(pool_files == nullptr){
                    pool_files = std::make_shared<spdlog::details::thread_pool>(buffer_size, 1);
                }
                    
                logger_files = std::make_unique<spdlog::async_logger>("logger_files",
                sink_files, pool_files, spdlog::async_overflow_policy::block);
                
                if(logger_files != nullptr){
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

void Logger::put_debug(const std::string& str){
    if(logger_level_console.load(std::memory_order_relaxed) == LOG_LEVEL_DEBUG){
        logger_stdout->debug(str);
    }
    
    if(logger_level_files.load(std::memory_order_relaxed) == LOG_LEVEL_DEBUG){
        logger_files->debug(str);
    }
}

void Logger::put_info(const std::string& str){
    if(logger_level_console.load(std::memory_order_relaxed) >= LOG_LEVEL_INFO){
        logger_stdout->info(str);
    }
    
    if(logger_level_files.load(std::memory_order_relaxed) >= LOG_LEVEL_INFO){
        logger_files->info(str);
    }
}

void Logger::put_warn(const std::string& str){
    if(logger_level_console.load(std::memory_order_relaxed) >= LOG_LEVEL_WARN){
        if(stderr_enabled.load(std::memory_order_relaxed)){
            logger_stderr->warn(str);
        }else{
            logger_stdout->warn(str);
        }
    }
    
    if(logger_level_files.load(std::memory_order_relaxed) >= LOG_LEVEL_WARN){
        logger_files->warn(str);
    }
}


void Logger::put_error(const std::string& str){
    if(logger_level_console.load(std::memory_order_relaxed) >= LOG_LEVEL_ERROR){
        if(stderr_enabled.load(std::memory_order_relaxed)){
            logger_stderr->error(str);
        }else{
            logger_stdout->error(str);
        }
    }
    
    if(logger_level_files.load(std::memory_order_relaxed) >= LOG_LEVEL_ERROR){
        logger_files->error(str);
    }
}


void Logger::put_critical(const std::string& str){
    if(logger_level_console.load(std::memory_order_relaxed) >= LOG_LEVEL_CRITICAL){
        if(stderr_enabled.load(std::memory_order_relaxed)){
            logger_stderr->error(str);
        }else{
            logger_stdout->error(str);
        }
    }
    
    if(logger_level_files.load(std::memory_order_relaxed) >= LOG_LEVEL_CRITICAL){
        logger_files->error(str);
    }
}


