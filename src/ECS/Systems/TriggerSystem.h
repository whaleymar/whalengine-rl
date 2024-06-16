#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Transform2D;
struct Velocity;
struct Trigger;
class Collider;

class TriggerSystem : public ecs::ISystem<Trigger> {
public:
    void update() override;
};

class MovableColliders : public ecs::ISystem<Transform2D, Velocity, Collider> {};

}  // namespace whal
