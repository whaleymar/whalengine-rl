#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct PlayerControl;
struct RigidBody;
struct Transform;
struct Velocity;
struct Jumper;

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
