#pragma once

#include "Gfx/Depth.h"
#include "Map/ComponentFactory.h"
#include "Physics/CollisionUtil.h"
#include "Physics/Material.h"
#include "Util/Vector.h"

namespace whal {

struct ParticleEmitter : ISerialize<ParticleEmitter, ComponentFactory> {
    WorldMaterial material;
    CollisionDir direction;
    Depth depth;
    s32 particlesPerSecond;
    f32 maxSpeed;
    f32 lifetimeMultiplier = 1.0;
    Vector2i aabbHalf = {1, 1};
    Vector2i offset = {0, 0};

    void setDirection(CollisionDir dir) { direction = dir; }

    static void loadImpl(ecs::Entity entity, void* data);
};

}  // namespace whal
