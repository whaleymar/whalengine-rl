#pragma once

#include <raylib.h>

#include "Physics/CollisionUtil.h"
#include "Util/Vector.h"

namespace whal {

namespace ParticleSetting {

enum Setting {
    None = 0,
    RigidBody = 1,
    Light = 1 << 1,
    UpOnly = 1 << 2,
    LeftOnly = 1 << 3,
    RightOnly = 1 << 4,
    DownOnly = 1 << 5,
    Collider = 1 << 6,
};

}

// RESEARCH would be cool to support sprite particles? Could be done once i consolidate those components
struct ParticleEmitter {
    Color color;
    f32 lifetimeSeconds;
    s32 settings;
    s32 particlesPerSecond;
    f32 maxSpeedTexelsPerSecond;
    Vector2i aabbHalfTexels = {1, 1};
    Vector2i offsetTexels = {0, 0};

    void setDirection(CollisionDir dir);
};

}  // namespace whal
