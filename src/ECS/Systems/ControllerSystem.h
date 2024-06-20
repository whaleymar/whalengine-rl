#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct PlayerControl;
struct FreeControl;
struct RigidBody;
struct Transform2D;
struct Velocity;
struct Jumper;

class ControllerSystem : public ecs::ISystem<PlayerControl, Transform2D, Velocity, RigidBody>, public ecs::IFixedUpdate {
public:
    void fixedUpdate() override;
};

class FreeControlSystem : public ecs::ISystem<PlayerControl, Transform2D, Velocity, FreeControl>, public ecs::IFixedUpdate {
public:
    void fixedUpdate() override;
};

class JumpSystem : public ecs::ISystem<Transform2D, Velocity, RigidBody, Jumper>, public ecs::IFixedUpdate {
public:
    void fixedUpdate() override;
};

}  // namespace whal
