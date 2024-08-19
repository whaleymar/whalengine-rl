#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct PlayerControl;
struct RigidBody;
struct Transform2D;
struct Velocity;
struct Jumper;

class ControllerSystem : public ecs::ISystem<PlayerControl, Transform2D, Velocity, RigidBody>, public ecs::IUpdate {
public:
    void update() override;
};

class FreeControlSystem : public ecs::ISystem<PlayerControl, Transform2D, Velocity, ecs::Exclude<RigidBody>>, public ecs::IUpdate {
public:
    void update() override;
};

class JumpSystem : public ecs::ISystem<Transform2D, Velocity, RigidBody, Jumper>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal
