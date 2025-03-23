#pragma once

#include "Components/PlayerControl.h"
#include "Components/RigidBody.h"
#include "Components/Transform.h"
#include "Components/Velocity.h"
#include "whalECS/src/ECS.h"

namespace whal {

class ControllerSystem : public ecs::ISystem<PlayerControl, Transform, Velocity, RigidBody>, public ecs::IUpdate {
public:
    void update() override;
};

class FreeControlSystem : public ecs::ISystem<PlayerControl, Transform, Velocity, ecs::Exclude<RigidBody>>, public ecs::IUpdate {
public:
    void update() override;
};

class JumpSystem : public ecs::ISystem<Transform, Velocity, RigidBody, Jumper>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal
