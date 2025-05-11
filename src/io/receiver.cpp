#include "receiver.h"

void Receiver::chown_chmod(){
    struct stat file_stat;
    if(stat(socket_path.c_str(), &file_stat))throw std::runtime_error("manager: fail to get socket owner/perms");

    struct passwd *pw = getpwnam(socket_owner.c_str());
    if (!pw)throw std::runtime_error("manager: no such user: " + socket_owner);
    uid_t desired_uid = pw->pw_uid;

    struct group *gr = getgrnam(socket_group.c_str());
    if (!gr)throw std::runtime_error("manager: no such group: " + socket_group);
    gid_t desired_gid = gr->gr_gid;

    mode_t desired_mode = socket_perms;
    desired_mode |= file_stat.st_mode & 0xF000;

    if(desired_uid != file_stat.st_uid || desired_gid != file_stat.st_gid){
        if(chown(socket_path.c_str(), desired_uid, desired_gid))throw std::runtime_error("manager: fail to change socket owner/group");
    }

    if(desired_mode != file_stat.st_mode){
        if(chmod(socket_path.c_str(), desired_mode))throw std::runtime_error("manager: fail to change socket permissions");
    }

}

Receiver::Receiver(const std::string& socket, const std::string& owner, const std::string& group, const int& perms)
: socket_path(socket), socket_owner(owner), socket_group(group), socket_perms(perms){
    is_running.store(false), thread_running.store(false);
    if(socket_path.empty() || socket_owner.empty() || socket_group.empty() || socket_perms < 0 || socket_perms > 511)throw std::runtime_error("manager: invalid args");
}

Receiver::~Receiver(){
//  if(is_running.load())receive_stop();
    socket->close();
    work_guard->reset();
    io->stop();
    
    while(thread_running.load())std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if(unlink(socket_path.c_str()))std::cerr << "manager: fail to unlink socket" << std::endl;
    delete work_guard;
    delete socket;
    delete io;
}

bool Receiver::now_running() const{
    return is_running.load() && thread_running.load();
}

void Receiver::init(const CallBack& cb){
    callback = cb;
    if(unlink(socket_path.c_str()) && errno != ENOENT)throw std::runtime_error("manager: fail to unlink socket");//TODO
//  if(unlink(socket_path.c_str()))throw std::runtime_error("manager: fail to unlink socket");//TODO
    
    io = new io_context;
    if(io == nullptr)throw std::runtime_error("manager: fail to create io_context");
    
    work_guard = new WorkGuard(asio::make_work_guard(*io));
    if(work_guard == nullptr)throw std::runtime_error("manager: fail to create io_context_guard");
    
    socket = new datagram_protocol::socket(*io, datagram_protocol());
    if(socket == nullptr)throw std::runtime_error("manager: fail to create socket");

    asio::error_code err;
    socket->bind(datagram_protocol::endpoint(socket_path), err);
    if(err)throw std::runtime_error("manager: fail to bind socket: " + err.message());
    
    chown_chmod();
}

void Receiver::receive_start(){
    if(is_running.load())throw std::runtime_error("manager: fail to start receiving (already start)");
    receive();
    is_running.store(true);
}

void Receiver::receive_stop(){
    if(!is_running)throw std::runtime_error("manager: fail to stop receiving (already stop)");
    asio::error_code err;
    socket->cancel(err);
    if(err)throw std::runtime_error("manager: fail to stop receiving: " + err.message());
    is_running.store(false);
}

void Receiver::io_run(){
    if(thread_running.load())throw std::runtime_error("manager: fail to start io (already start)");
    thread_running.store(true);
    io_thread = std::thread([this]() {io->run(); thread_running.store(false);});
    io_thread.detach();
}

void Receiver::receive(){
    socket->async_receive_from(
        asio::buffer(buffer),
        endpoint,
        [this](const asio::error_code& err, const std::size_t& recv_bytes){
            if(err == asio::error::operation_aborted){
                std::cerr << "manager: async io operation aborted (receiving stop)" << std::endl;
                return;
            }
            else if(!err)callback(std::string(buffer.data(), recv_bytes), endpoint);
            else std::cerr << "manager: fail to receive data: " << err.message() << std::endl;
            receive();
        }
    );
}

