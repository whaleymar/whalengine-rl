#pragma once

#include "Util/Singleton.h"
#include "Util/Types.h"

namespace whal {

struct System;

class TimeManager {
    SINGLETON(TimeManager)
public:
    friend System;

    f32 dt() const { return mDeltatime; }  // shorthand
    f32 getDeltaTime() const { return mDeltatime; }
    f32 getUnmodified() const { return mDeltatimeUnmodified; }
    f32 getElapsed() const { return mTimeElapsed; }                      // updated once per frame
    f32 getElapsedUnmodified() const { return mTimeElapsedUnmodified; }  // updated once per frame
    s32 getFrame() const { return mFrame; }
    void setMultiplier(f32);
    f32 getMultiplier() const { return mTimeMultiplier; }
    void sleep(int milliseconds);
    f32 getElapsedPrecise() const;  // precise time within frame

private:
    void update();
    f32 mDeltatime = 0.01;
    f32 mDeltatimeUnmodified = 0.01;
    f32 mTimeMultiplier = 1.0;
    f32 mTimeElapsed = 0.0;
    f32 mTimeElapsedUnmodified = 0.0;
    s32 mFrame = 0;
};

}  // namespace whal
