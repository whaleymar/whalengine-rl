#include "Deltatime.h"

#include <raylib.h>

#include <chrono>
#include <thread>

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

void Deltatime::sleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

}  // namespace whal
