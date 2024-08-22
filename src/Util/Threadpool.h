#pragma once

#include "Settings.h"

#ifdef USE_THREADS

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<void>;

private:
    std::vector<std::thread> mWorkers;
    std::queue<std::function<void()>> mTasks;

    std::mutex mQueueMutex;
    std::condition_variable mCondition;
    bool mIsStopping;
};

inline ThreadPool::ThreadPool(size_t numThreads) : mIsStopping(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        mWorkers.emplace_back([this] {
            for (;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->mQueueMutex);
                    this->mCondition.wait(lock, [this] { return this->mIsStopping || !this->mTasks.empty(); });
                    if (this->mIsStopping && this->mTasks.empty())
                        return;
                    task = std::move(this->mTasks.front());
                    this->mTasks.pop();
                }
                task();
            }
        });
    }
}

inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(mQueueMutex);
        mIsStopping = true;
    }
    mCondition.notify_all();
    for (std::thread& worker : mWorkers)
        worker.join();
}

template <class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<void> {
    auto task = std::make_shared<std::packaged_task<void()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<void> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(mQueueMutex);
        if (mIsStopping) {
        } else {
            mTasks.emplace([task]() { (*task)(); });
        }
    }
    mCondition.notify_one();
    return res;
}

#endif
