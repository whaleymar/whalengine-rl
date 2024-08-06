#pragma once

#include "Util/Types.h"
#include "Util/Vector.h"

#include <raylib.h>

namespace whal {

struct IEmitLight {
    s32 radiusTexels = 1;
    s32 heightTexels = 0;  // offset from transform
    Color color = WHITE;
};

// lights have a multiplicative effect on other objects (determines how visible they are).
// radiance is additive. its color stays the same but its transparency increases with distance. Plus it affects the background.

// this is implemented in a kind of jank way, but it's fast
struct PointLight : public IEmitLight {};

// slower than pointlight, but more control over shape
struct BoxLight : public IEmitLight {
    Vector2i halfLenTexels;

    static BoxLight PointLight(s32 radiusTexels, s32 heightTexels, Color color = WHITE) {
        return BoxLight{{radiusTexels, heightTexels, color}, {0, 0}};
    }
};

struct Radiance : public IEmitLight {};

struct ShadowLight : public IEmitLight {};

}  // namespace whal
