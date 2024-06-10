#pragma once

#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <type_traits>

#include "Util/Types.h"

namespace {

using BoundFunction = std::function<void()>;
using Job = std::pair<BoundFunction, f32>;

}  // namespace

namespace whal {

namespace evfl {

struct Node {
    // RESEARCH:
    // union {
    //     f32 waitSeconds;
    //     s32 waitFrames;
    // };
    f32 waitSeconds = 0.0;
    std::function<void()> boundFunc = nullptr;
    std::unique_ptr<Node> next = nullptr;
};

// RESEARCH
// extension: branching logic
// `struct BoolNode : public Node` stores a callback which returns a bool
// and this bool decides which child path is taken
// (implementation: Nodes get a virtual getNext() method to do this)
class EventFlow {
public:
    EventFlow(bool isPaused = false) : mIsPaused(isPaused) {}

    template <typename... T>
    EventFlow& add(std::type_identity_t<std::function<void(T...)>> const& func, T... args);

    EventFlow& addWait(f32 waitSeconds);

    void tick(f32 deltaTime);
    bool isDone() const { return mRoot == nullptr; }
    bool isPaused() const { return mIsPaused; }

private:
    std::unique_ptr<Node> mRoot = nullptr;
    Node* mEnd = nullptr;
    bool mIsPaused;
};

template <typename... T>
EventFlow& EventFlow::add(std::type_identity_t<std::function<void(T...)>> const& func, T... args) {
    if (!func) {
        return *this;
    }
    BoundFunction bf = std::bind(func, args...);  // boyfriend :3
    auto pNode = std::make_unique<Node>(0, bf, nullptr);
    if (mRoot == nullptr) {
        mRoot = std::move(pNode);
        mEnd = mRoot.get();
        return *this;
    }

    mEnd->next = std::move(pNode);
    mEnd = mEnd->next.get();
    return *this;
}

}  // namespace evfl

struct System;

class JobScheduler {
public:
    friend System;

    void start();
    void await();
    void end();

    template <typename... T>
    void after(std::type_identity_t<std::function<void(T...)>> const& func, f32 delaySeconds, T... args);

    evfl::EventFlow& eventFlow(bool isPaused = false);

    void tick(f32 dt);
    void tryExecuteJobs();

private:
    JobScheduler() = default;

    void worker();

    std::mutex mMutex;
    std::thread mJobThread;
    std::condition_variable mCondition;

    std::list<Job> mQueue;
    std::vector<evfl::EventFlow> mEventFlows;

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
