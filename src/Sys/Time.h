#pragma once

#include "Util/Types.h"

namespace whal {

struct System;

class Time {
public:
    friend System;

    f32 getDeltaTime() const { return mDeltatime; }
    f32 getUnmodified() const { return mDeltatimeUnmodified; }
    f32 getElapsed() const { return mTimeElapsed; }
    s32 getFrame() const { return mFrame; }
    void update();
    void setMultiplier(f32);
    f32 getMultiplier() const { return mTimeMultiplier; }
    void sleep(int milliseconds);

private:
    Time();
    Time(const Time&) = delete;
    void operator=(const Time&) = delete;

    f32 mDeltatime = 0.01;
    f32 mDeltatimeUnmodified = 0.01;
    f32 mTimeMultiplier = 1.0;
    f32 mTimeElapsed = 0.0;
    s32 mFrame = 0;
};

}  // namespace whal
