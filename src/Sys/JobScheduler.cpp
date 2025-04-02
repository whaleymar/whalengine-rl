#include "JobScheduler.h"

#include <initializer_list>
#include <memory>
#include "Events/Events.h"
#include "System.h"

namespace whal {

namespace evfl {

static u32 EVFL_ID = 1;  // 0 is invalid ID

EventFlow::EventFlow(u32 id, std::initializer_list<ecs::Entity> requiredEntities) : mRequiredEntities(requiredEntities), mId(id) {
    for (ecs::Entity e : mRequiredEntities) {
        if (e.isKilledThisFrame()) {
            mIsCancelled = true;
            invalidate();
            break;
        }
    }
}

EventFlow& EventFlow::addWait(f32 waitSeconds) {
    if (mIsCancelled || waitSeconds == 0.0f) {
        return *this;
    }

    auto pNode = std::make_unique<Node>(nullptr, waitSeconds);
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
    if (mRoot == nullptr || mIsCancelled) {
        return;
    }

    if (mRoot->tick(deltaTime)) {
        mRoot = std::move(mRoot->next);
    }
}

}  // namespace evfl

// Entity death listener callback
void checkEventFlows(ecs::Entity entity) {
    for (auto& evflow : Schedule.getEventFlows()) {
        if (evflow.requiresEntity(entity)) {
            evflow.invalidate();
        }
    }

    // update tween manager
    Schedule.getTweenMgr().onEntityKilled(entity);
}

JobScheduler::JobScheduler() : mDeathListener(&checkEventFlows) {
    Event.registerListener<evt::Death>(mDeathListener);
}

void JobScheduler::start() {
#ifdef USE_THREADS
    mJobThread = std::thread(&JobScheduler::worker, this);
#endif
}

void JobScheduler::await() {
#ifdef USE_THREADS
    mJobThread.join();
#endif
}

void JobScheduler::end() {
    mIsTerminated = true;
#ifdef USE_THREADS
    mCondition.notify_one();
#endif
}

void JobScheduler::clear() {
    mEventFlowsToAdd.clear();
    mEventFlows.clear();

#ifdef USE_THREADS
    if (mQueue.size() > 0) {
        std::unique_lock<std::mutex> lock(mMutex);
        mCondition.wait(lock, [this] { return (!mQueue.empty() && mQueue.begin()->second <= 0) || mIsTerminated; });
        mQueue.clear();
    }
#else
    mQueue.clear();
#endif

    // clear tweens
    mTweenMgr.clear();
}

evfl::EventFlow& JobScheduler::flow(std::initializer_list<ecs::Entity> requiredEntities) {
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
#ifdef USE_THREADS
    mCondition.notify_one();
#else
    tryExecuteJobs();
#endif

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

    // update tweens
    mTweenMgr.update();
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

#ifdef USE_THREADS
void JobScheduler::worker() {
    while (!mIsTerminated) {
        std::unique_lock<std::mutex> lock(mMutex);
        mCondition.wait(lock, [this] { return (!mQueue.empty() && mQueue.begin()->second <= 0) || mIsTerminated; });

        tryExecuteJobs();
    }
}
#endif

}  // namespace whal
