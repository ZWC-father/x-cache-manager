#include "algo_lru.h"
#include <cstdio>
#include <iostream>
#include <cstring>
void rb_cb(std::vector<LRU::Cache> removed){
    for(auto iter : removed){
        std::cerr << "Removed: " << iter.first << " Size: " << iter.second << std::endl;
    }
}
int main(int argc, char** argv){
    LRU* lru = new LRU(100, rb_cb);
    char key[100];
    size_t size;
    while(~scanf("%s%llu", key, &size)){
        if(std::string(key) == "exit")break;
        else if(std::string(key) == "resize")lru->resize(size);
        else lru->put(std::string(key), size);
        memset(key, 0, sizeof key);
        lru->display();
    }
    std::cerr << "ok" << std::endl;
    delete lru;
    return 0;
}
