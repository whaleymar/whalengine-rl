#pragma once

#include "Physics/Material.h"
#include "Util/Types.h"
#include "Util/Vector.h"

namespace whal {

// TODO parameters like jumpInitialVelocity, jumpSecondsMax, and coyoteTimeSecondsMax are jump-specific. Could be a second "Jumper" component that
// relies on rigidbody

struct RigidBody {
    RigidBody() = default;
    RigidBody(f32 jumpInitialVelocity_, f32 jumpSecondsMax_, f32 coyoteTimeSecondsMax_);

    void setGrounded(WorldMaterial material);
    void setNotGrounded();

    f32 jumpInitialVelocity = 124;
    f32 jumpSecondsMax = 1.25;
    f32 coyoteTimeSecondsMax = 0.1;
    Vector2f momentumDamping = {1.0, 0.5};

    // automatically managed:
    f32 jumpSecondsRemaining = 0;
    f32 coyoteSecondsRemaining = 0;
    s32 momentumCooldownFrames = 0;
    WorldMaterial groundMaterial = WorldMaterial::None;
    bool isJumping = false;
    bool isLanding = false;
    bool isGrounded = false;
    // TODO define jump height & derive velocity from that
};

}  // namespace whal
