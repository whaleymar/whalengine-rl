#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Transform2D;
struct Velocity;
// struct Trigger;
struct TriggerZone;
struct TriggerCircle;
class ActorCollider;

// class TriggerSystem : public ecs::ISystem<Trigger> {
// public:
//     void update() override;
// };

class TriggerSystem : public ecs::ISystem<TriggerZone> {
public:
    void update() override;
};

class TriggerCircleSystem : public ecs::ISystem<TriggerCircle> {
public:
    void update() override;
};

class MovableActorTracker : public ecs::ISystem<Transform2D, Velocity, ActorCollider> {};

}  // namespace whal
