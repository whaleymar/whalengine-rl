#pragma once

#include "Util/Types.h"

#include <raylib.h>

namespace whal {

struct IEmitLight {
    s32 radius = 1;
    s32 height = 0;  // offset from transform
    Color color = WHITE;
};

// lights have a multiplicative effect on other objects (determined how visible they are).
// radiance is additive. its color stays the same but its transparency increases with distance. Plus it affects the background.

struct PointLight : public IEmitLight {};

struct Radiance : public IEmitLight {};

// struct EnvironmentLight : public IEmitLight {
//     s32 radius;
// }

}  // namespace whal
