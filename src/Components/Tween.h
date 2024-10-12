#pragma once

#include "Sys/System.h"
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

class TweenManager;

// *might* want an onDelete callback for if the entity is killed during the tween and some global value needs resetting
template <typename T>
class Tween {
    friend TweenManager;

public:
    using ValueGetter = T& (*)(ecs::Entity);
    using TweenCallback = void (*)(ecs::Entity, const Tween<T>&);

    Tween(T target, f32 duration, ValueGetter getter) : mDuration(duration), mTweenValue(target), mGetter(getter) {}

    f32 getProgress() const { return (mElapsedTime - mDelay) / mDuration; }
    T getValue() const { return ease(mStartValue, mEndValue, getProgress(), mEaseFunc); }

    // for casting (not rounding)
    template <typename NewType>
    NewType getValueAs() const {
        return ease(static_cast<NewType>(mStartValue), static_cast<NewType>(mEndValue), getProgress(), mEaseFunc);
    }

    void tick(ecs::Entity entity) {
        const f32 dt = System::isPaused() && !isSet(TweenParams::IgnorePause) ? 0.0f :
                       isSet(TweenParams::IgnoreSlowdown)                     ? System::time.getUnmodified() :
                                                                                System::dt();

        if (mDelay > mElapsedTime) {
            mElapsedTime += dt;
            return;
        }

        if (mGetter) {
            mGetter(entity) = getValue();
        }

        if (getProgress() >= 1.0) {
            if (mNumLoops != 0) {
                const s32 nLoopsRemaining = mNumLoops == -1 ? -1 : mNumLoops - 1;
                setLoops(nLoopsRemaining);
                mElapsedTime = mDelay;  // delay only affects first iteration
                resetFlag(TweenParams::CustomOrigin);
                if (isSet(TweenParams::Bounce)) {
                    mTweenValue *= -1.0f;
                }
                init(entity);
            } else {
                mIsDone = true;
            }

        } else {
            mElapsedTime += dt;
        }
    }

    void onEnd(ecs::Entity entity) {
        if (mOnEnd) {
            mOnEnd(entity, *this);
        }
    }

    void onUpdate(ecs::Entity entity) {
        if (mOnUpdate) {
            mOnUpdate(entity, *this);
        }
    }

    bool isDone() const { return mIsDone; }

    Tween<T>& setOnEnd(TweenCallback onEnd_) {
        mOnEnd = onEnd_;
        return *this;
    }

    Tween<T>& setOnUpdate(TweenCallback onUpdate_) {
        mOnUpdate = onUpdate_;
        return *this;
    }

    Tween<T>& setDelay(f32 delay = 0.0) {
        mDelay = delay;
        return *this;
    }

    Tween<T>& asRelative() {
        setFlag(TweenParams::RelativeTarget);
        return *this;
    }

    Tween<T>& asBounce() {
        setFlag(TweenParams::Bounce);
        return *this;
    }

    Tween<T>& asRunDuringPause() {
        setFlag(TweenParams::IgnorePause);
        return *this;
    }

    Tween<T>& asIgnoreSlowdown() {
        setFlag(TweenParams::IgnoreSlowdown);
        return *this;
    }

    Tween<T>& setLoops(s32 n = 0) {
        mNumLoops = n;
        return *this;
    }

    Tween<T>& from(T start) {
        setFlag(TweenParams::CustomOrigin);
        mStartValue = start;
        return *this;
    }

    Tween<T>& setTransition(Ease easeFunc) {
        mEaseFunc = easeFunc;
        return *this;
    }

    bool isSet(TweenParams::Flags flag) { return (mFlags & flag) > 0; }
    void resetFlag(TweenParams::Flags flag) { mFlags = (mFlags & ~flag); }
    void setFlag(TweenParams::Flags flag) { mFlags = (mFlags | flag); }

private:
    void init(ecs::Entity entity) {
        T val = isSet(TweenParams::CustomOrigin) ? mStartValue : mGetter(entity);
        mStartValue = val;
        if (isSet(TweenParams::RelativeTarget)) {
            mEndValue = val + mTweenValue;
        } else {
            mEndValue = mTweenValue;
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
    ValueGetter mGetter;
    TweenCallback mOnEnd = nullptr;
    TweenCallback mOnUpdate = nullptr;
    u8 mFlags = TweenParams::None;
    bool mIsDone = false;
};

typedef Tween<f32> TweenFloat;
typedef Tween<s32> TweenInt;
typedef Tween<Vector2f> TweenVec2f;
typedef Tween<Vector2i> TweenVec2i;
typedef Tween<Color> TweenColor;

// I'm just gonna say "please don't do anything that can kill an entity in these callbacks"
class TweenManager : public IListen<DeathEvent, false, ecs::Entity> {
    struct TweenLists {
        std::vector<TweenFloat> floats;
        std::vector<TweenInt> ints;
        std::vector<TweenVec2f> vec2fs;
        std::vector<TweenVec2i> vec2is;
        std::vector<TweenColor> colors;

        bool isEmpty() const { return colors.empty() && floats.empty() && ints.empty() && vec2fs.empty() && vec2is.empty(); }
    };

public:
    static TweenManager& instance() {
        static TweenManager instance_;
        return instance_;
    }

    // kill tweens for that entity
    void onEvent(DeathEvent, ecs::Entity entity) override;
    void update();

    static void add(TweenFloat tween, ecs::Entity entity) {
        instance().addAndInit(tween, entity);
        instance().mTweens.at(entity).floats.push_back(tween);
    }

    static void add(TweenInt tween, ecs::Entity entity) {
        instance().addAndInit(tween, entity);
        instance().mTweens.at(entity).ints.push_back(tween);
    }

    static void add(TweenVec2f tween, ecs::Entity entity) {
        instance().addAndInit(tween, entity);
        instance().mTweens.at(entity).vec2fs.push_back(tween);
    }

    static void add(TweenVec2i tween, ecs::Entity entity) {
        instance().addAndInit(tween, entity);
        instance().mTweens.at(entity).vec2is.push_back(tween);
    }

    static void add(TweenColor tween, ecs::Entity entity) {
        instance().addAndInit(tween, entity);
        instance().mTweens.at(entity).colors.push_back(tween);
    }

private:
    TweenManager() = default;
    TweenManager(TweenManager&) = delete;
    TweenManager(TweenManager&&) = delete;

    template <typename T>
    void addAndInit(Tween<T>& tween, ecs::Entity entity) {
        if (!mTweens.contains(entity)) {
            mTweens[entity] = TweenLists();
        }
        tween.init(entity);
    }

    std::unordered_map<ecs::Entity, TweenLists, ecs::EntityHash> mTweens;
};

}  // namespace whal
