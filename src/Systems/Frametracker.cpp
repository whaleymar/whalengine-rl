#include "Frametracker.h"

#include <chrono>

namespace whal {

Frametracker::Frametracker() : mPreviousFrame1Time(std::chrono::steady_clock::now()) {}

f32 Frametracker::update() {
    mFrame++;
    if (mFrame != mMaxCount) {
        return mFPS;
    }
    mFrame = 0;
    auto newTime = std::chrono::steady_clock::now();
    mFPS = static_cast<f32>(mMaxCount) / std::chrono::duration<f32>(newTime - mPreviousFrame1Time).count();
    mPreviousFrame1Time = newTime;
    return mFPS;
}

}  // namespace whal
