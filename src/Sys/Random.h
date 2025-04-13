#pragma once

#include "Util/Types.h"

namespace whal {

struct System;

// Not a singleton! Free to make new RNG managers for game stuff!
class RNGManager {
public:
    RNGManager();
    RNGManager(u32 seed) : mState(seed) {}
    f32 uniform();

    f32 range(f32 lower, f32 upper);
    s32 range(s32 lower, s32 upperExclusive);

private:
    u32 mState;
};

}  // namespace whal
