#include <iostream>
#include "config_parser.h"
int main(){
    ConfigParser* conf = new ConfigParser("conf.yaml");
    int port = conf->get_required<int>("redis.port");
    std::string host = conf->get_required<std::string>("redis.host");
    auto path = conf->get_optional<std::string>("sqlite.path", "cache.db");
    auto timeout = conf->get_array<int>("redis.timeout");

    std::cout << host << " " << port << " " << path << std::endl;
    for(auto it : timeout)std::cout << it << std::endl;
    return 0;
}
