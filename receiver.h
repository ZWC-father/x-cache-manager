#include <cerrno>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <array>
#include <chrono>
#include <thread>
#include <atomic>

#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <asio.hpp>
#include <asio/error_code.hpp>
#include <asio/local/datagram_protocol.hpp>

#define BUFFER_SIZE 4096

using asio::local::datagram_protocol;
using asio::io_context;

class Receiver{
public:
     using CallBack = std::function<void(const std::string& data, const datagram_protocol::endpoint& ep)>;
     Receiver(const std::string& socket, const std::string& owner, const std::string& group, const int& perms);
     ~Receiver();
     void init(const CallBack& cb);
     void io_run();
     void receive_start();
     void receive_stop();
     bool now_running() const;


private:
     const std::string socket_path;
     const std::string socket_owner;
     const std::string socket_group;
     const int socket_perms;
     
     std::array<char, BUFFER_SIZE> buffer;
     datagram_protocol::endpoint endpoint;
     CallBack callback;
     
     using WorkGuard = asio::executor_work_guard<io_context::executor_type>;
     io_context* io = nullptr;
     WorkGuard* work_guard = nullptr;
     datagram_protocol::socket* socket = nullptr;

     std::thread io_thread;
     std::atomic<bool> thread_running = false, is_running = false;//TODO: add is_init

     void chown_chmod();
     void receive();
};
