#pragma once

typedef float f32;

namespace whal {

struct System;

class Deltatime {
public:
    friend System;

    f32 operator()() const { return mDeltatime; }
    f32 getUnmodified() const { return mDeltatimeUnmodified; }
    f32 getElapsed() const { return mTimeElapsed; }
    void update();
    void setMultiplier(f32);

private:
    Deltatime();
    Deltatime(const Deltatime&) = delete;
    void operator=(const Deltatime&) = delete;

    f32 mDeltatime = 0.01;
    f32 mDeltatimeUnmodified = 0.01;
    f32 mTimeMultiplier = 1.0;
    f32 mTimeElapsed = 0.0;
};

}  // namespace whal
