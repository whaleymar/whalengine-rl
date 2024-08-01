#include "Time.h"

#include <raylib.h>
#include "Settings.h"

namespace whal {

static constexpr f32 MAX_FRAME_TIME = 0.1;  // cap at half a second

Time::Time() {}

void Time::update() {
    f32 frameTime = GetFrameTime();
    mDeltatimeUnmodified = frameTime > MAX_FRAME_TIME ? MAX_FRAME_TIME : frameTime;
    mDeltatime = mDeltatimeUnmodified * mTimeMultiplier;
    mTimeElapsed += mDeltatime;

    mFrame++;
    if (mFrame != FPS_TARGET) {
        return;
    }
    mFrame = 0;
}

void Time::setMultiplier(f32 multiplier) {
    mTimeMultiplier = multiplier;
}

}  // namespace whal
