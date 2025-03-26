#pragma once

#include "Util/Vector.h"

namespace whal {

// velocity in pixels per second
struct Velocity {
    static Velocity from(Vector2f stableVelocity) {
        return Velocity{
            .stable = stableVelocity,
            .impulse = {},
            .total = {},
            .residualImpulse = {},
        };
    }

    Vector2f stable;
    Vector2f impulse;
    Vector2f total;  // for tracking true speed

    // immediately going from impulse -> nothing feels very jarring. this is used to have a smoother transition
    Vector2f residualImpulse;
};

// positive is clockwise, negative is counterclockwise
struct AngularVelocity {
    f32 rotationsPerSecond = 0;
    static AngularVelocity fromSecondsPerRotation(f32 secondsPerRotation) { return AngularVelocity{1.0f / secondsPerRotation}; }
};

}  // namespace whal
