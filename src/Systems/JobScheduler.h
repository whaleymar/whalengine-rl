#pragma once

#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <type_traits>

#include "Util/Types.h"

namespace whal {

struct System;

class JobScheduler {
    using BoundFunction = std::function<void()>;
    using Job = std::pair<BoundFunction, f32>;

public:
    friend System;

    void start();
    void await();
    void end();

    template <typename... T>
    void after(std::type_identity_t<std::function<void(T...)>> const& func, f32 delaySeconds, T... args);

    void tick(f32 dt);
    void tryExecuteJobs();

private:
    JobScheduler() = default;

    void worker();

    std::mutex mMutex;
    std::thread mJobThread;
    std::condition_variable mCondition;
    std::list<Job> mQueue;

    bool mIsTerminated = false;
};

template <typename... T>
void JobScheduler::after(std::type_identity_t<std::function<void(T...)>> const& func, f32 delaySeconds, T... args) {
    auto it = mQueue.begin();
    while (it != mQueue.end() && it->second < delaySeconds) {
        ++it;
    }
    BoundFunction bf = std::bind(func, args...);  // boyfriend :3
    mQueue.insert(it, {bf, delaySeconds});
}
}  // namespace whal
