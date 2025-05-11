#include "redis_adapter.h"
#include <iomanip>
#include <cstdint>

int main(){
    int a;
    int64_t b;
    uint64_t c;
    std::string d;
    std::cin >> a >> b >> c >> d;
    std::string res = RedisAdapter::serialize(a, b, c, d);
    std::cout << "Raw: ";
    std::ios_base::fmtflags oldFlags = std::cout.flags();
    for(unsigned char it : res){
        std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(it) << " ";
    }
    int aa;
    int64_t bb;
    uint64_t cc;
    std::string dd;
    std::cout.flags(oldFlags);
    std::cout << "Res: " << RedisAdapter::deserialize(res, aa, bb, cc, dd) << " ";
    std::cout << aa << " " << bb << " " << cc << " " << dd << std::endl;

    return 0;
}
