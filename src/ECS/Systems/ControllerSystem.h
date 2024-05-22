#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct PlayerControlRB;
struct PlayerControlFree;
struct RigidBody;
struct Transform2D;
struct Velocity;

class ControllerSystemRB : public ecs::ISystem<PlayerControlRB, Transform2D, Velocity, RigidBody> {
public:
    void update() override;
};

class ControllerSystemFree : public ecs::ISystem<PlayerControlFree, Transform2D, Velocity> {
public:
    void update() override;
};

}  // namespace whal
