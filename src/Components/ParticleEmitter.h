#pragma once

#include <raylib.h>

#include "Physics/CollisionUtil.h"
#include "Physics/Material.h"
#include "Util/Vector.h"

namespace whal {

struct ParticleEmitter {
    WorldMaterial material;
    CollisionDir direction;
    s32 particlesPerSecond;
    f32 maxSpeedTexelsPerSecond;
    Vector2i aabbHalfTexels = {1, 1};
    Vector2i offsetTexels = {0, 0};

    void setDirection(CollisionDir dir) { direction = dir; }
};

}  // namespace whal
