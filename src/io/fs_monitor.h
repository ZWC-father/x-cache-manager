#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <string>
#include <memory>
#include <stdexcept>
#include <cstdint>
#include <condition_variable>
#include <functional>

#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>

class MonitorError : public std::runtime_error{
public:
    explicit MonitorError(const std::string& err) : std::runtime_error(err) {}
};

class Monitor{
public:
    using CallBack = std::function<void(const std::string, int)>;
    Monitor(const std::string& dir, CallBack cb);
    

private:
    unordered_set<std::string> dirs;
    CallBack callback;


};
