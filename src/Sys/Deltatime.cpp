#include "Deltatime.h"

#include <raylib.h>

namespace whal {

static constexpr f32 MAX_FRAME_TIME = 0.5;  // cap at half a second

Deltatime::Deltatime() {}

void Deltatime::update() {
    f32 frameTime = GetFrameTime();
    mDeltatimeUnmodified = frameTime > MAX_FRAME_TIME ? MAX_FRAME_TIME : frameTime;
    mDeltatime = mDeltatimeUnmodified * mTimeMultiplier;
}

void Deltatime::setMultiplier(f32 multiplier) {
    mTimeMultiplier = multiplier;
}

}  // namespace whal
