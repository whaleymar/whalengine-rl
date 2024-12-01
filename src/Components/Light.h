#pragma once

#include "Gfx/Color.h"
#include "Util/Types.h"
#include "Util/Vector.h"

namespace whal {

struct IEmitLight {
    s32 radius = 1;
    s32 heightOffset = 0;
    Color color = Colors::White;
};

// TODO get rid of the inheritance, use dependency injection

// this is implemented in a kind of jank way, but it's fast
struct PointLight : public IEmitLight {};

// slower than pointlight, but more control over shape
struct BoxLight : public IEmitLight {
    Vector2i halfLen;
};

struct ShadowLight : public IEmitLight {};

}  // namespace whal
