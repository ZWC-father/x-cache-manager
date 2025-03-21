#include "parser.h"
#include "receiver.h"

struct LogValuePrinter {
    void operator()(std::nullptr_t) const { std::cout << "NULL"; }
    void operator()(bool b) const { std::cout << (b ? "True" : "False"); }
    void operator()(int64_t i) const { std::cout << i; }
    void operator()(uint64_t u) const { std::cout << u; }
    void operator()(double u) const { std::cout << u; }
    void operator()(std::string d) const { std::cout << d; }
    void operator()(Parser::UnknownType uk) const {
        if(uk.exist)std::cout << "Unknown Type Value: " << uk.value;
        else std::cout << "N/A";
    }
};

std::vector<std::string> keys = {"time", "remote_addr", "request", "status", "cache_size", "body_bytes_sent", "user_agent"};
Parser parse(keys);
void callback(const std::string& data, const asio::local::datagram_protocol::endpoint& ep){
    std::cout << data << std::endl << std::endl;
    auto res = parse.parse(data);
    for(int i = 0; i < res.size(); i++){
        std::cout << "key: " << keys[i] << std::endl;
        std::visit(LogValuePrinter(), res[i]);
        std::cout << std::endl;
    }

    std::cout<<std::endl;
    
}

int main(){
    Receiver* recv = new Receiver(std::string("/var/run/nginx_access.sock"), std::string("nginx"), std::string("nginx"), 511);

    recv->init(callback);
    recv->io_run();
    

    std::cout<<"start"<<std::endl;
    recv->receive_start();
    std::this_thread::sleep_for(std::chrono::seconds(114514));
    delete recv;
    

    return 0;
    

}


