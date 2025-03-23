#pragma once

#include "Components/Velocity.h"
#include "Events/Events.h"
#include "Physics/HitInfo.h"
#include "Sys/IListen.h"
#include "whalECS/src/ECS.h"

#include "Util/Types.h"

namespace whal {

struct PhysicsParams {
    f32 gravity = 100;  // px/s^2
    f32 terminalVelocityY = -160;
    f32 frictionGround = 240;
    f32 frictionAir = 200;
    f32 moveEpsilon = 0.1;  // how low a residual impulse force should be before it's zeroed
    f32 jumpPeakGravityMult = 0.5;
    f32 jumpPeakSpeedMax = -28;  // once Y velocity is below this, no longer considered "jumping"
};

class PhysicsSystem : public ecs::ISystem<Transform, Velocity>, public ecs::IUpdate, public IListen<evt::Collision, true, ecs::Entity, HitInfo> {
public:
    PhysicsSystem();
    void update() override;
    void onEvent(evt::Collision, ecs::Entity, HitInfo) override;
};

class RotationPhysicsSystem : public ecs::ISystem<Transform, AngularVelocity>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal
