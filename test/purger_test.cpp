#include "purger.h"
#include <string>

void process_fail(const std::string path, int tries){
    std::cerr << "fuck" << std::endl; 
}
int main(int argc, char **agrv){
    Purger* purger = new Purger(process_fail,
                                std::string("http://127.0.0.1/"), 
                                std::string("PURGE"),
                                256,
                                4,
                                3,
                                200);

    std::string tmp;
    while(1){
        std::getline(std::cin, tmp);
        if(tmp == "exit")break;
        std::cerr<<"ok"<<std::endl;
        purger->add(tmp);
    }
    purger->stop();
    delete purger;
    return 0;
}

