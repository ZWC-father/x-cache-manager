#include "purger.h"

void curl_easy_basic_opt(CURL* curl, long timeout){
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);
//TODO:add source ip address support
}

size_t write_callback(void* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

Purger::Purger(FailCallback cb,
               const std::string& srv,
               const std::string& cmd,
               size_t q_max,
               int th_max,
               int t_max,
               long tim) :
               fail_callback(cb), purge_command(cmd),
               queue_max(q_max), thread_max(th_max), try_max(t_max),
               timeout(tim)
{ 
    if(srv.back() == '/')server_name = srv.substr(0, srv.size() - 1);
    else server_name = srv;
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    for(int i = 0; i < thread_max; i++){
        auto thread_ptr = std::make_unique<std::thread>(&Purger::worker, this);
        thread_ptr->detach();
        running_worker++;
        std::cerr << "purge worker #" << i << " detached" << std::endl;
    }
}

Purger::~Purger(){
    stop();
    curl_global_cleanup();
}

void Purger::worker(){
    CURL* curl = curl_easy_init();
    CURLcode curl_code;
    long http_code;
    std::string buffer;

    curl_easy_basic_opt(curl, timeout);

    while(true){
        std::unique_lock<std::mutex>uni_lock(queue_lock);
        if(purge_queue.empty() && !stop_signal){
            worker_cv.wait(uni_lock,
                           [this]{return stop_signal || purge_queue.size();});
        }
        
        if(stop_signal){
            curl_easy_cleanup(curl);
            running_worker--;
            return;
        }

        auto task = purge_queue.front();
        purge_queue.pop();
        uni_lock.unlock();
        task.second++;

        curl_easy_setopt(curl, CURLOPT_URL, (server_name + task.first).c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, purge_command.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        
        curl_code = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if(http_code != 200 || buffer.substr(0, 4) != VALID_RESPONSE){
            std::cerr << "purge failed: " << server_name + task.first << std::endl;
            std::cerr << "http code: " << http_code << ", response: " << buffer << std::endl;
            if(task.second < try_max){
                uni_lock.lock();
                if(purge_queue.size() < queue_max){
                    std::cerr << "retry: " << server_name + task.first << std::endl;
                    purge_queue.push(task);
                    uni_lock.unlock();
                    worker_cv.notify_one();
                }else{
                    std::cerr << "queue overflow!" << std::endl;
                    fail_callback(task.first, task.second);
                }
            }else{
                fail_callback(task.first, task.second);
            }
        }else{
            std::cerr << "success: " << buffer << std::endl;
        }

        buffer.clear();
    }
}

bool Purger::add(const std::string& path){
    std::unique_lock<std::mutex>uni_lock(queue_lock);
    if(purge_queue.size() < queue_max){
        if(path.front() == '/')purge_queue.emplace(path, 0);
        else purge_queue.emplace('/' + path, 0);
        uni_lock.unlock();
        worker_cv.notify_one();
        return 1;
    }
    std::cerr << "queue overflow!" << std::endl;
    return 0;    
}

size_t Purger::size() const{
    std::lock_guard<std::mutex>lock_gd(queue_lock);
    return purge_queue.size();
}

void Purger::stop(){
    if(stop_signal)return;
    
    queue_lock.lock();
    stop_signal = true;
    queue_lock.unlock();
    worker_cv.notify_all();

    while(running_worker)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    //TODO: verify the memory order (all of the worker must quit with once notify)

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

