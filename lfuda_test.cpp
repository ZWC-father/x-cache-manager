#include "algo_lfuda_simple.h"
#include <string>
void remove_callback(std::vector<LFUDA::Cache> removed){
    for(auto it : removed){
        std::cerr << "Removed: " << it.key << std::endl;
    }
}
int main(int argc, char** argv){
    if(argc < 2)exit(1);
    LFUDA* algo = new LFUDA(std::stoull(std::string(argv[1])), remove_callback);
    char key[64] = {0};
    size_t size;
    while(~scanf("%s%lu", key, &size)){
        if(std::string(key) == "exit")break;
        if(std::string(key) == "resize")algo->resize(size);
        else algo->put(std::string(key), size, 0);
        algo->display();
    }
    delete algo;
    return 0;
}
