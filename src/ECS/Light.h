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

struct PointLight : public IEmitLight {};

// struct EnvironmentLight : public IEmitLight {
//     s32 radius;
// }

}  // namespace whal
