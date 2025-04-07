#include "logger.h"
#include <exception>
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>


void Logger::set_logger(int type, int action, const std::string& file,
                        size_t rotating_size, size_t rotating_nums){
    try{
        if(type & LOG_SINK_STDOUT){
            if(action == LOG_ACTION_INIT){
                if(logger_stdout == nullptr){
                    if(sink_stdout == nullptr)sink_stdout = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
                    sink_stdout->set_color(spdlog::level::debug, "\033[32m");
                    sink_stdout->set_color(spdlog::level::info,  "\033[37m");
                    sink_stdout->set_color(spdlog::level::warn,  "\033[33m");
                    sink_stdout->set_color(spdlog::level::err,   "\033[31m");
                    sink_stdout->set_color(spdlog::level::critical, "\033[35m");
                    logger_stdout = std::make_unique<spdlog::logger>("logger_stdout", sink_stdout);
                    logger_stdout->set_error_handler([](const std::string& msg){
                          throw LoggerError("Logger [STDOUT] Error while Logging: " + msg);
                    });

                }else LoggerError("Logger [STDOUT] Duplicated Setup");
            } 
        }

        if(type & LOG_SINK_STDERR){
            if(action == LOG_ACTION_INIT){
                if(logger_stderr == nullptr){
                }else LoggerError("Logger [STDERR] Duplicated Setup");
            }

        }

        if(type & LOG_SINK_FILES){

        }
    }catch(const std::exception& e){
        throw LoggerError(std::string("Logger Setup Error: ") + e.what());
    }
}


