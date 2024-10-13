#pragma once

#include <raylib.h>

#include "Gfx/Depth.h"
#include "Physics/CollisionUtil.h"
#include "Physics/Material.h"
#include "Util/Vector.h"

namespace whal {

struct ParticleEmitter {
    WorldMaterial material;
    CollisionDir direction;
    Depth depth;
    s32 particlesPerSecond;
    f32 maxSpeed;
    f32 lifetimeMultiplier = 1.0;
    Vector2i aabbHalf = {1, 1};
    Vector2i offset = {0, 0};

    void setDirection(CollisionDir dir) { direction = dir; }
};

}  // namespace whal
