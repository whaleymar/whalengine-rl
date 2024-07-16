#include "JobScheduler.h"

#include <initializer_list>
#include <memory>
#include "Events/Events.h"
#include "System.h"

namespace whal {

namespace evfl {

static u32 EVFL_ID = 1;  // 0 is invalid ID

EventFlow::EventFlow(u32 id, std::initializer_list<ecs::Entity> requiredEntities) : mRequiredEntities(requiredEntities), mId(id) {}

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

void checkEventFlows(ecs::Entity entity) {
    for (auto& evflow : System::schedule.getEventFlows()) {
        if (evflow.requiresEntity(entity)) {
            evflow.invalidate();
        }
    }
}

JobScheduler::JobScheduler() : mDeathListener(&checkEventFlows) {
    System::eventMgr.registerListener<DeathEvent>(mDeathListener);
}

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

evfl::EventFlow& JobScheduler::eventFlow(std::initializer_list<ecs::Entity> requiredEntities) {
    mEventFlowsToAdd.push_back(evfl::EventFlow(evfl::EVFL_ID++, requiredEntities));
    return mEventFlowsToAdd.back();
}

void JobScheduler::cancelEventFlow(u32 id) {
    if (id == 0) {
        return;
    }
    for (auto it = mEventFlows.begin(); it != mEventFlows.end(); it++) {
        if (it->getId() == id) {
            mEventFlows.erase(it);
            return;
        }
    }
    for (auto it = mEventFlowsToAdd.begin(); it != mEventFlowsToAdd.end(); it++) {
        if (it->getId() == id) {
            mEventFlowsToAdd.erase(it);
            return;
        }
    }
}

void JobScheduler::tick(f32 dt) {
    for (auto it = mQueue.begin(); it != mQueue.end(); ++it) {
        it->second -= dt;
    }
    mCondition.notify_one();

    // add queued event flows to main collection
    for (auto& evflow : mEventFlowsToAdd) {
        mEventFlows.push_back(std::move(evflow));
    }
    mEventFlowsToAdd.clear();

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
