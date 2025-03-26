#include <chrono>
#include <hiredis/read.h>
#include <iostream>
#include <mutex>
#include <ratio>
#include <stdexcept>
#include <thread>
#include <functional>
#include <utility>
#include <hiredis/hiredis.h>

#define PORT 6379
#define SRC_ADDR "127.0.0.1"
#define CONN_TIME 1000
#define CMD_TIME 1000
#define ALIVE_INTVL 30000
#define RETRY_INTVL 200
#define MAX_RETRY 3

class RedisError : public std::runtime_error{
public:
    explicit RedisError(const std::string& err) : std::runtime_error(err) {}
};

class RedisBase{
public:
    struct ErrorCount{
        int io_error, eof_error, timeout_error, other_error;
        void add(int error_flag){
            if(error_flag == REDIS_ERR_IO)io_error++;
            else if(error_flag == REDIS_ERR_EOF)eof_error++;
            else if(error_flag == REDIS_ERR_TIMEOUT)timeout_error++;
            else other_error++;
        }
    }error_count;
    
    RedisBase(const std::string& h, int p, const std::string& src_a, int con_t,
              int cmd_t, int al_int, int rt_int, int max_rt);
    ~RedisBase();
    
    void connect();
    void disconnect();


private:
    std::thread connect_daemo;
    redisOptions redis_opt;
    redisContext* redis_ctx;
    
    std::string host, source_addr;
    int port;
    timeval connect_tv, command_tv;
    int alive_interval, max_retries;
    std::chrono::milliseconds retry_interval;

    template<typename Fn>
    auto do_redis(Fn&& fn)
    -> decltype(fn()){
        for(int i = 1; i <= max_retries; i++){
            try{
                return fn();
            }catch(const RedisError& e){
                std::cerr << e.what() << " (attempt #" << i <<
                " failed)" << std::endl;
                if(i == max_retries)throw;
            }
            std::this_thread::sleep_for(retry_interval);
        }
    }
    
    void reconnect();
    void perror(const std::string& error_msg);
    void reset_count();
   
};
