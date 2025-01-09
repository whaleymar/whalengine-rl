#pragma once

#include "Map/ComponentFactory.h"
#include "Map/TiledParse.h"
#include "Physics/Material.h"
#include "Settings.h"
#include "Util/Types.h"
#include "Util/Vector.h"

namespace whal {

struct RigidBody : ISerialize<RigidBody, ComponentFactory> {
    void setGrounded(WorldMaterial material);
    void setNotGrounded();

    Vector2f frictionMultiplier = {1.0, 1.0};  // x is ground, y is air
    Vector2f momentumMultiplier = {1.0, 0.5};
    f32 gravityMultiplier = WORLD_TYPE == WorldType2D::SideScroller ? 1.0 : 0.0;

    // automatically managed:
    s32 framesSinceLanding = 0;
    WorldMaterial groundMaterial = WorldMaterial::None;
    bool isLanding = false;
    bool isGrounded = false;
};

}  // namespace whal
