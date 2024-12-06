#pragma once

#include "Map/ComponentFactory.h"
#include "Map/TiledParse.h"
#include "Util/Vector.h"

namespace whal {

// velocity in pixels per second
struct Velocity : ISerialize<Velocity, ComponentFactory> {
    Velocity() = default;
    Velocity(Vector2f velocity) : stable(velocity) {}

    Vector2f stable;
    Vector2f impulse;
    Vector2f total;  // for tracking true speed

    // immediately going from impulse -> nothing feels very jarring. this is used to have a smoother transition
    Vector2f residualImpulse;

    static void loadImpl(ecs::Entity entity, void* data) {
        const LoadContext& ctx = *static_cast<LoadContext*>(data);
        Velocity velocity = entity.has<Velocity>() ? entity.get<Velocity>() : Velocity{};
        tryRead(ctx.values, "stable", &velocity.stable);

        entity.add(velocity);
    }
};

// positive is clockwise, negative is counterclockwise
struct AngularVelocity {
    f32 rotationsPerSecond = 0;
    static AngularVelocity fromSecondsPerRotation(f32 secondsPerRotation) { return AngularVelocity{1.0f / secondsPerRotation}; }
};

}  // namespace whal
