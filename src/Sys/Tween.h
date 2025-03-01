#pragma once

#include <functional>
#include <memory>
#include "Util/Easing.h"
#include "whalECS/src/ECS.h"

namespace whal {

namespace TweenParams {
enum Flags : u8 {
    None = 0,
    IgnoreSlowdown = 1,
    RelativeTarget = 1 << 1,
    CustomOrigin = 1 << 2,
    Bounce = 1 << 3,
    IgnorePause = 1 << 4,
};
}

template <typename T>
concept Multipliable = requires(T a, T b) {
    a* b;  // Check if the multiplication operator is defined
};

class TweenManager;

class ITween {
public:
    friend TweenManager;
    virtual ~ITween() {}
    virtual bool isStarted() const = 0;
    virtual bool isDone() const = 0;
    virtual bool isCancelled() const = 0;
    virtual ecs::Entity getEntity() const = 0;
    virtual void kill() = 0;
    virtual bool isSet(TweenParams::Flags flag) const = 0;

private:
    virtual bool isDelayCondition() = 0;
    virtual void init() = 0;
    virtual void tick(f32 dt) = 0;
    virtual void onStart() const = 0;
    virtual void onUpdate() const = 0;
    virtual void onEnd() const = 0;
};

template <typename T>
class Tween;

// A handler wrapper for tweens which holds a (counted) reference to the tween, while
// exposing tween methods to the user via "." notation instead of pointer notation
template <typename T>
class Tweener {
public:
    using TweenCallback = void (*)(ecs::Entity, const Tween<T>&);
    using BoolTweenCallback = bool (*)(ecs::Entity, const Tween<T>&);

    Tweener() = default;
    Tweener(std::shared_ptr<Tween<T>> tween) : mTween(tween) {}

    void kill() const {
        if (mTween) {
            mTween->kill();
        }
    }
    bool isDone() const { return mTween && mTween->isDone(); }

    Tweener<T>& setOnStart(TweenCallback onStart_) {
        mTween->mOnStart = onStart_;
        return *this;
    }

    Tweener<T>& setOnEnd(TweenCallback onEnd_) {
        mTween->mOnEnd = onEnd_;
        return *this;
    }

    Tweener<T>& setOnUpdate(TweenCallback onUpdate_) {
        mTween->mOnUpdate = onUpdate_;
        return *this;
    }

    Tweener<T>& setDelayCondition(BoolTweenCallback delayCB) {
        mTween->mDelayCallback = delayCB;
        return *this;
    }

    Tweener<T>& setDelay(f32 delay = 0.0) {
        mTween->mDelay = delay;
        return *this;
    }

    Tweener<T>& asRelative() {
        mTween->setFlag(TweenParams::RelativeTarget);
        return *this;
    }

    Tweener<T>& asBounce() {
        mTween->setFlag(TweenParams::Bounce);
        if (mTween->mNumLoops == 0) {
            // make sure we have at least 1 loop for the bounce
            mTween->mNumLoops = 1;
        }
        return *this;
    }

    Tweener<T>& asRunDuringPause() {
        mTween->setFlag(TweenParams::IgnorePause);
        return *this;
    }

    Tweener<T>& asIgnoreSlowdown() {
        mTween->setFlag(TweenParams::IgnoreSlowdown);
        return *this;
    }

    Tweener<T>& setLoops(s32 n = 0) {
        mTween->mNumLoops = n;
        return *this;
    }

    Tweener<T>& from(T start) {
        mTween->setFlag(TweenParams::CustomOrigin);
        mTween->mStartValue = start;
        mTween->setValue(start);
        return *this;
    }

    Tweener<T>& setTransition(Ease easeFunc) {
        mTween->mEaseFunc = easeFunc;
        return *this;
    }

    const Tween<T>* get() const { return mTween.get(); }

private:
    std::shared_ptr<Tween<T>> mTween;
};

class JobScheduler;
struct System;
class TweenManager {
public:
    friend System;
    friend JobScheduler;

    template <typename T>
    using ValueGetter = std::type_identity_t<std::function<T&(ecs::Entity)>>;
    // using ValueGetter = T& (*)(ecs::Entity);

    template <typename T>
    using ValueSetter = std::type_identity_t<std::function<void(const T&, ecs::Entity)>>;

    // called automatically
    void onEntityKilled(ecs::Entity e);

private:
    TweenManager() = default;
    TweenManager(TweenManager&) = delete;
    TweenManager operator=(TweenManager&) = delete;

    // Managed by System:
    void update();
    void clear();

    std::vector<std::shared_ptr<ITween>> mTweens;
    std::unordered_set<ecs::Entity, ecs::EntityHash> mKilledEntities;
};

// *might* want an onDelete callback for if the entity is killed during the tween and some global value needs resetting
template <typename T>
class Tween : public ITween {
    friend TweenManager;
    friend Tweener<T>;

public:
    using TweenCallback = void (*)(ecs::Entity, const Tween<T>&);
    using BoolTweenCallback = bool (*)(ecs::Entity, const Tween<T>&);

    // i would like this to be private but friending std::shared_ptr doesn't work
    Tween(T target, f32 duration, TweenManager::ValueGetter<T> getter, ecs::Entity entity)
        : mDuration(duration), mTweenValue(target), mGetter(getter), mEntity(entity) {}

    Tween(T target, f32 duration, TweenManager::ValueGetter<T> getter, TweenManager::ValueSetter<T> setter, ecs::Entity entity)
        : mDuration(duration), mTweenValue(target), mGetter(getter), mSetter(setter), mEntity(entity) {}

    ~Tween() = default;

    f32 getProgress() const { return (mElapsedTime - mDelay) / mDuration; }
    T getValue() const { return ease(mStartValue, mEndValue, math::clamp(getProgress(), 0.0f, 1.0f), mEaseFunc); }
    f32 getDuration() const { return mDuration; }

    // for casting (not rounding)
    template <typename NewType>
    NewType getValueAs() const {
        return ease(static_cast<NewType>(mStartValue), static_cast<NewType>(mEndValue), math::clamp(getProgress(), 0.0f, 1.0f), mEaseFunc);
    }

    ecs::Entity getEntity() const override { return mEntity; }
    bool isStarted() const override { return mIsStarted; }
    bool isDone() const override { return mIsDone; }
    bool isCancelled() const override { return mIsCancelled; }
    void kill() override { mIsCancelled = true; }
    bool isSet(TweenParams::Flags flag) const override { return (mFlags & flag) > 0; }

private:
    void tick(f32 dt) override {
        if (mDelay > mElapsedTime) {
            mElapsedTime += dt;
            return;
        }
        if (mIsCancelled || mIsDone) {
            return;
        }
        mIsStarted = true;

        setValue(getValue());

        if (getProgress() >= 1.0) {
            if (mNumLoops != 0) {
                const s32 nLoopsRemaining = mNumLoops == -1 ? -1 : mNumLoops - 1;
                setLoops(nLoopsRemaining);
                mElapsedTime = mDelay;  // delay only affects first iteration
                resetFlag(TweenParams::CustomOrigin);
                if (isSet(TweenParams::Bounce)) {
                    if (isSet(TweenParams::RelativeTarget)) {
                        mTweenValue = mTweenValue * -1.0f;
                    } else {
                        auto tmp = mTweenValue;
                        mTweenValue = mStartValue;
                        mStartValue = tmp;
                    }
                }
                init();
            } else {
                mIsDone = true;
            }

        } else {
            mElapsedTime += dt;
        }
    }

    void onStart() const override {
        if (mOnStart) {
            mOnStart(mEntity, *this);
        }
    }

    void onEnd() const override {
        if (mOnEnd) {
            mOnEnd(mEntity, *this);
        }
    }

    void onUpdate() const override {
        if (mOnUpdate) {
            mOnUpdate(mEntity, *this);
        }
    }

    bool isDelayCondition() override {
        if (mDelayCallback == nullptr) {
            return false;
        }
        bool isDelayed = mDelayCallback(mEntity, *this);
        if (!isDelayed) {
            mDelayCallback = nullptr;
        }
        return isDelayed;
    }

    void setLoops(s32 n = 0) { mNumLoops = n; }
    void resetFlag(TweenParams::Flags flag) { mFlags = (mFlags & ~flag); }
    void setFlag(TweenParams::Flags flag) { mFlags = (mFlags | flag); }

    void init() override {
        if (!isSet(TweenParams::CustomOrigin)) {
            mStartValue = mGetter(mEntity);
        }

        if (isSet(TweenParams::RelativeTarget)) {
            mEndValue = mStartValue + mTweenValue;
        } else {
            mEndValue = mTweenValue;
        }
    }

    void setValue(const T& val) {
        if (mSetter) {
            mSetter(val, mEntity);
        } else {
            mGetter(mEntity) = val;
        }
    }

    f32 mDuration;
    Ease mEaseFunc = Ease::Linear;
    f32 mElapsedTime = 0.0f;
    f32 mDelay = 0.0f;
    s32 mNumLoops = 0;
    T mStartValue;
    T mEndValue;
    T mTweenValue;
    std::function<T&(ecs::Entity)> mGetter;
    std::function<void(const T&, ecs::Entity)> mSetter = nullptr;
    ecs::Entity mEntity;
    TweenCallback mOnStart = nullptr;
    TweenCallback mOnEnd = nullptr;
    TweenCallback mOnUpdate = nullptr;
    BoolTweenCallback mDelayCallback = nullptr;
    u8 mFlags = TweenParams::None;
    bool mIsDone = false;
    bool mIsStarted = false;
    bool mIsCancelled = false;
};

}  // namespace whal
