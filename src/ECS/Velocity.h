#pragma once

#include "Util/Vector.h"

namespace whal {

// velocity in texels per second
struct Velocity {
    Velocity() = default;
    Velocity(Vector2f velocity);

    Vector2f stable;
    Vector2f impulse;
    Vector2f total;  // for tracking true speed

    // immediately going from impulse -> nothing feels very jarring. this is used to have a smoother transition
    Vector2f residualImpulse;
};

}  // namespace whal
