#include "async_deleter.h"
#include <system_error>
void fail_cb(const std::string& file, std::error_code ec) noexcept{
    std::cout << file << std::endl;
}

int main(){
    AsyncDeleter ad(2, fail_cb);
    ad.run();
    ad.submit("tmpfile");
    ad.stop();
    return 0;
}
