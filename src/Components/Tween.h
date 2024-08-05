#pragma once

#include "Util/Easing.h"

namespace whal {

class TweenPositionSystem;

// TODO
// TweenColor (replace ColorLerp)
// TweenAlpha (replace FadeOut)
// TweenScale (replace complex animation brain stuff)
// TweenRotate

namespace TweenParams {
enum Flags : u8 {
    None = 0,
    IgnoreSlowdown = 1,
    RelativeTarget = 1 << 1,
    CustomOrigin = 1 << 2,
    Bounce = 1 << 3,
};

}

// TODO instead of TweenPosition child, should have an onRemove callback which can add a new tween
struct TweenPosition {
    friend TweenPositionSystem;

    Vector2i target;
    f32 duration;
    Ease easing = Ease::Linear;

    TweenPosition() = default;
    TweenPosition(Vector2i target_, f32 duration_, Ease ease_) : target(target_), duration(duration_), easing(ease_) {}

    TweenPosition setDelay(f32 delay = 0.0) {
        mDelay = delay;
        return *this;
    }

    TweenPosition asRelative() {
        setFlag(TweenParams::RelativeTarget);
        return *this;
    }

    TweenPosition asBounce() {
        setFlag(TweenParams::Bounce);
        return *this;
    }

    TweenPosition asRunDuringPause() {
        setFlag(TweenParams::IgnoreSlowdown);
        return *this;
    }

    TweenPosition setLoops(s32 n = 0) {
        mNumLoops = n;
        return *this;
    }

    TweenPosition from(Vector2i origin) {
        setFlag(TweenParams::CustomOrigin);
        mStartPosition = origin.as<f32>();
        return *this;
    }

    bool isSet(TweenParams::Flags flag) { return (mFlags & flag) > 0; }
    void resetFlag(TweenParams::Flags flag) { mFlags = (mFlags & ~flag); }

private:
    void setFlag(TweenParams::Flags flag) { mFlags = (mFlags | flag); }
    f32 mElapsedTime = 0.0;
    f32 mDelay = 0.0;
    Vector2f mStartPosition;
    Vector2f mEndPosition;
    s32 mNumLoops = 0;
    u8 mFlags = TweenParams::None;
};

}  // namespace whal
