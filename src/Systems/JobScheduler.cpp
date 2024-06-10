#include "JobScheduler.h"

#include <memory>

namespace whal {

namespace evfl {

EventFlow& EventFlow::addWait(f32 waitSeconds) {
    auto pNode = std::make_unique<Node>(waitSeconds, nullptr, nullptr);
    if (mRoot == nullptr) {
        mRoot = std::move(pNode);
        mEnd = mRoot.get();
        return *this;
    }

    mEnd->next = std::move(pNode);
    mEnd = mEnd->next.get();
    return *this;
}

// run root node or wait
void EventFlow::tick(f32 deltaTime) {
    if (mRoot == nullptr) {
        return;
    }
    if (mRoot->waitSeconds > 0) {
        mRoot->waitSeconds -= deltaTime;
        return;
    }

    if (mRoot->boundFunc) {
        mRoot->boundFunc();
    }
    mRoot = std::move(mRoot->next);
}

}  // namespace evfl

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

evfl::EventFlow& JobScheduler::eventFlow(bool isPaused) {
    mEventFlows.push_back(evfl::EventFlow(isPaused));
    return mEventFlows.back();
}

void JobScheduler::tick(f32 dt) {
    for (auto it = mQueue.begin(); it != mQueue.end(); ++it) {
        it->second -= dt;
    }
    mCondition.notify_one();

    auto it = mEventFlows.begin();
    while (it != mEventFlows.end()) {
        if (it->isDone()) {
            it = mEventFlows.erase(it);
        } else {
            it->tick(dt);
            ++it;
        }
    }
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
