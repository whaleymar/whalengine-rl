#include "Deltatime.h"

#include <raylib.h>

namespace whal {

Deltatime::Deltatime() {}

void Deltatime::update() {
    mDeltatimeUnmodified = GetFrameTime();
    mDeltatime = mDeltatimeUnmodified * mTimeMultiplier;
}

void Deltatime::setMultiplier(f32 multiplier) {
    mTimeMultiplier = multiplier;
}

}  // namespace whal
