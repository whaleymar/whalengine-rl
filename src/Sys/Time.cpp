#include "Time.h"

#include <chrono>
#include <raylib.h>
#include <thread>
#include "Settings.h"

namespace whal {

static constexpr f32 MAX_FRAME_TIME = 0.1;  // cap at half a second

TimeManager::TimeManager() {}

void TimeManager::update() {
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

void TimeManager::setMultiplier(f32 multiplier) {
    mTimeMultiplier = multiplier;
}

void TimeManager::sleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

}  // namespace whal
