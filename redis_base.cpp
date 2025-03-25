#include "redis_base.h"
#include <hiredis/hiredis.h>
RedisBase::RedisBase(const std::string& host, int port = PORT, 
const std::string& source_addr = SRC_ADDR, int connect_timeout = CONN_TIME,
int command_timeout = CMD_TIME, int alive_interval = CHK_INTVL,
int max_retries = MAX_RETRY) : host(host), port(port),
source_addr(source_addr), alive_interval(alive_interval),
max_retries(max_retries), redis_opt({0}){
    
    redis_opt.options |= REDIS_OPT_PREFER_IP_UNSPEC;
    REDIS_OPTIONS_SET_TCP(&redis_opt, host.c_str(), port);
    redis_opt.endpoint.tcp.source_addr = source_addr.c_str();
    connect_tv = {connect_timeout / 1000,
                  connect_timeout % 1000 * 1000};
    command_tv = {command_timeout / 1000,
                  command_timeout % 1000 * 1000};
    redis_opt.connect_timeout = &connect_tv;
    redis_opt.command_timeout = &command_tv; 

}

bool RedisBase::connect(){
    redis_ctx = redisConnectWithOptions(&redis_opt);
}

