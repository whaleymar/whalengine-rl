#pragma once

#include "Util/Types.h"

namespace whal {

struct LightColor {
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;
};

static const LightColor LIGHTCOLOR_WHITE = {255, 255, 255, 255};

struct IEmitLight {
    s32 radius;
    s32 height;  // offset from transform
    LightColor color = LIGHTCOLOR_WHITE;
};

// lights have a multiplicative effect on other objects (determined how visible they are).
// radiance is additive. its color stays the same but its transparency increases with distance. Plus it affects the background.

struct PointLight : public IEmitLight {};

struct Radiance : public IEmitLight {};

// struct EnvironmentLight : public IEmitLight {
//     s32 radius;
// }

}  // namespace whal
