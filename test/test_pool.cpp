#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <future>
#include <vector>
#include <chrono>

class FixedTaskThreadPool {
public:
    // 构造函数：创建指定数量的工作线程
    FixedTaskThreadPool(size_t thread_count) : stop(false), done(false) {
        for (size_t i = 0; i < thread_count; ++i) {
            // 使用 std::jthread 自动管理线程生命周期，并利用 stop_token 控制退出
            workers.emplace_back([this](std::stop_token token) {
                while (true) {
                    Task task;
                    {
                        std::unique_lock lock(queue_mutex);
                        // 修改等待条件，除了等待任务到来外，还等待 done 标志
                        condition.wait(lock, [this, &token]() {
                            return token.stop_requested() || !tasks.empty() || done;
                        });
                        // 如果收到停止请求或 finish() 被调用，并且队列为空，则退出循环
                        if ((token.stop_requested() || done) && tasks.empty())
                            break;
                        // 弹出队列中的任务
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    // 执行固定任务（本例中计算整数的平方）
                    int result = processTask(task.param);
                    // 将任务结果通过 promise 返回给调用者
                    task.promise.set_value(result);
                }
            });
        }
    }
    
    // 禁止拷贝和赋值操作
    FixedTaskThreadPool(const FixedTaskThreadPool&) = delete;
    FixedTaskThreadPool& operator=(const FixedTaskThreadPool&) = delete;
    
    // 提交任务：传入一个整数参数，返回 future<int> 供调用者阻塞等待结果
    std::future<int> submit(int param) {
        std::promise<int> promise;
        auto future = promise.get_future();
        {
            std::lock_guard lock(queue_mutex);
            if (stop)
                throw std::runtime_error("线程池已停止，无法提交新任务");
            tasks.push(Task{ param, std::move(promise) });
        }
        condition.notify_one();
        return future;
    }
    
    // 调用 finish() 表示不再提交新任务，所有工作线程在处理完剩余任务后会自动退出
    void finish() {
        {
            std::lock_guard lock(queue_mutex);
            done = true;
        }
        condition.notify_all();
    }
    
    // 析构函数中调用 finish() 并设置 stop 标志，确保工作线程能安全退出
    ~FixedTaskThreadPool() {
        finish();
        {
            std::lock_guard lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        // 这里不用显式 join，std::jthread 在析构时会自动发出停止请求并 join
    }
    
private:
    // 内部任务结构，包含任务参数和与之关联的 promise 对象
    struct Task {
        int param;
        std::promise<int> promise;
    };
    
    // 内部固定任务函数，用于处理任务，本例中为计算输入参数的平方
    int processTask(int param) {
        // 模拟耗时处理
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return param * param;
    }
    
    std::vector<std::jthread> workers;       // 工作线程集合（利用 jthread 自动管理生命周期）
    std::queue<Task> tasks;                  // 任务队列，存储固定格式的任务
    std::mutex queue_mutex;                  // 互斥量保护任务队列
    std::condition_variable condition;       // 条件变量用于线程等待任务到来或 finish/停止信号
    bool stop;                               // 停止标志（由析构函数设置，防止新的任务提交）
    bool done;                               // 标记不再提交新任务，达到自动退出线程的条件
};

int main() {
    FixedTaskThreadPool pool(4);
    
    // 提交多个任务进行处理（计算输入参数的平方）
    auto future1 = pool.submit(3);
    auto future2 = pool.submit(4);
    auto future3 = pool.submit(5);
    
    std::cout << "等待任务完成并获取结果...\n";
    std::cout << "3的平方: " << future1.get() << "\n";
    std::cout << "4的平方: " << future2.get() << "\n";
    std::cout << "5的平方: " << future3.get() << "\n";
    
    // 通知线程池不再提交新任务，空闲工作线程在处理完剩余任务后将自动退出
    pool.finish();
    
    // main 函数结束后，pool 的析构函数会被调用，所有线程会自动 join，从而程序能自动终止
    return 0;
}

