#pragma once

#include "Gfx/Color.h"
#include "Map/ComponentFactory.h"
#include "Util/Types.h"
#include "Util/Vector.h"

namespace whal {

// this is implemented in a kind of jank way, but it's fast
struct PointLight : ISerialize<PointLight, ComponentFactory> {
    s32 radius = 1;
    s32 heightOffset = 0;
    Color color = Colors::White;
};

// slower than pointlight, but more control over shape
struct BoxLight {
    s32 radius = 1;
    s32 heightOffset = 0;
    Color color = Colors::White;
    Vector2i halfLen;
};

struct ShadowLight {
    s32 radius = 1;
    s32 heightOffset = 0;
    Color color = Colors::White;
};

}  // namespace whal
