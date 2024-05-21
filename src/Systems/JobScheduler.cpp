#include "JobScheduler.h"

namespace whal {

void JobScheduler::start() {
    mJobThread = std::thread(&JobScheduler::worker, this);
}

void JobScheduler::await() {
    mJobThread.join();
}

void JobScheduler::end() {
    mIsTerminated = true;
    mCondition.notify_one();
}

void JobScheduler::tick(f32 dt) {
    for (auto it = mQueue.begin(); it != mQueue.end(); ++it) {
        it->second -= dt;
    }
    mCondition.notify_one();
}

void JobScheduler::tryExecuteJobs() {
    auto it = mQueue.begin();
    while (it != mQueue.end()) {
        Job job = *it;
        if (job.second > 0) {
            break;
        }
        job.first();
        it = mQueue.erase(it);
        if (it != mQueue.end()) {
            ++it;
        }
    }
}

void JobScheduler::worker() {
    while (!mIsTerminated) {
        std::unique_lock<std::mutex> lock(mMutex);
        mCondition.wait(lock, [this] { return (!mQueue.empty() && mQueue.begin()->second <= 0) || mIsTerminated; });

        tryExecuteJobs();
    }
}

}  // namespace whal
