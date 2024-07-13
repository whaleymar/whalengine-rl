#pragma once

#include "Physics/Material.h"
#include "Util/Types.h"
#include "Util/Vector.h"

namespace whal {

struct RigidBody {
    RigidBody() = default;
    RigidBody(Vector2f frictionMult) : frictionMultiplier(frictionMult) {}

    void setGrounded(WorldMaterial material);
    void setNotGrounded();

    Vector2f frictionMultiplier = {1.0, 1.0};  // x is ground, y is air
    Vector2f momentumMultiplier = {1.0, 0.5};
    f32 gravityMultiplier = 1.0;

    // automatically managed:
    s32 momentumCooldownFrames = 0;
    s32 framesSinceLanding = 0;
    WorldMaterial groundMaterial = WorldMaterial::None;
    bool isLanding = false;
    bool isGrounded = false;
};

}  // namespace whal
