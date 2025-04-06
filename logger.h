#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include "spdlog/spdlog.h"

#define LOG_CONSOLE_STDOUT 1
#define LOG_CONSOLE_STDERR 2
#define LOG_FILE 4

#define LOG_DISABLE 0
#define LOG_ENABLE 1

#define LOG_OPT_STRING_MAX 0
#define LOG_OPT_QUEUE_MAX 1
#define LOG_OPT_HOLDERS_MAX 2
#define LOG_OPT_ROTATING_SIZE 3
#define LOG_OPT_ROTATING_NUMS 4

class LoggerError : public std::runtime_error{
public: explicit LoggerError(const std::string& err) : std::runtime_error(err) {}
};

class Logger{
public:
    Logger();
    ~Logger();
    void setup(int type, bool action, const std::string& file = "");
    template<typename T>
    void setopt(int opt, T arg){
        if(opt == 0){
        }else if(opt == 1){
        }else if(opt == 2){
        }else if(opt == 3){
        }else if(opt == 4){
        }else{
            throw LoggerError("logger error: unknown option");
        }
    }
    static void put_debug();
    static void put_info();
    static void put_warn();
    static void put_errr();
    static void put_fatal();

private:
    int holder_id;
    

};
