#pragma once

#include "Settings.h"

#ifdef USE_THREADS

#include <condition_variable>
#include <mutex>
#include <thread>

#else

#include <memory>

#endif

#include <functional>
#include <initializer_list>
#include <list>
#include <type_traits>

#include "Event.h"
#include "Events/Events.h"
#include "Tween.h"
#include "whalECS/src/ECS.h"

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
    EventFlow(u32 id, std::initializer_list<ecs::Entity> requiredEntities = {});

    template <typename... T>
    EventFlow& add(std::type_identity_t<std::function<void(T...)>> const& func, T... args);

    // RESEARCh -- requires BoolNode
    // template <typename... T>
    // EventFlow& cancelIf(std::type_identity_t<std::function<bool(T...)>> const& func, T... args);

    EventFlow& addWait(f32 waitSeconds);

    void tick(f32 deltaTime);
    bool isDone() const { return mRoot == nullptr; }
    u32 getId() const { return mId; }

    bool requiresEntity(ecs::Entity entity) {
        if (ecs::whal_find(mRequiredEntities.begin(), mRequiredEntities.end(), entity) != mRequiredEntities.end()) {
            return true;
        }
        return false;
    }

    void invalidate() { mRoot.release(); }

private:
    std::unique_ptr<Node> mRoot = nullptr;
    Node* mEnd = nullptr;
    std::vector<ecs::Entity> mRequiredEntities;
    u32 mId;
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

// This can't inherit IListen because circular imports
class JobScheduler {
public:
    friend System;

    JobScheduler();

    // Asych callback scheduling:
    template <typename... T>
    void after(std::type_identity_t<std::function<void(T...)>> const& func, f32 delaySeconds, T... args);

    // Serial event flow scheduling:
    evfl::EventFlow& flow(std::initializer_list<ecs::Entity> requiredEntities = {});
    void cancelEventFlow(u32 id);
    std::vector<evfl::EventFlow>& getEventFlows() { return mEventFlows; }
    TweenManager& getTweenMgr() { return mTweenMgr; }

    // Tween scheduling
    // have to use `auto` for the getter, otherwise the compiler can't infer T for some reason.
    // static_cast still enforces compile-time type safety.
    template <typename T>
    Tweener<T> tween(ecs::Entity entity, T target, f32 duration, auto getter) {
        std::shared_ptr<Tween<T>> tween = std::make_shared<Tween<T>>(target, duration, static_cast<TweenManager::ValueGetter<T>>(getter), entity);
        mTweenMgr.mTweens.push_back(tween);
        return Tweener(tween);
    }

    // example usage: `Schedule.tween(entity, 360.0f, 2, &Transform::rotation)`
    template <typename Component, typename T>
        requires Multipliable<T>
    Tweener<T> tween(ecs::Entity entity, auto target, f32 duration, T Component::*member) {
        const auto getter = [member](ecs::Entity e) -> T& { return e.get<Component>().*member; };
        std::shared_ptr<Tween<T>> tween = std::make_shared<Tween<T>>(target, duration, getter, entity);
        mTweenMgr.mTweens.push_back(tween);
        return Tweener(tween);
    }

private:
    JobScheduler(const JobScheduler&) = delete;
    void operator=(const JobScheduler&) = delete;

    void start();
    void await();
    void end();
    void clear();
    void tick(f32 dt);
    void tryExecuteJobs();

#ifdef USE_THREADS
    void worker();

    std::mutex mMutex;
    std::thread mJobThread;
    std::condition_variable mCondition;
#endif

    TweenManager mTweenMgr;
    std::list<Job> mQueue;
    std::vector<evfl::EventFlow> mEventFlows;
    std::vector<evfl::EventFlow> mEventFlowsToAdd;
    EventListener<ecs::Entity> mDeathListener;  // Halts EventFlows which use killed entities

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
