#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Transform2D;
struct Velocity;
struct TriggerZone;
class ActorCollider;

class TriggerSystem : public ecs::ISystem<TriggerZone> {
public:
    void update() override;
};

class MovableActorTracker : public ecs::ISystem<Transform2D, Velocity, ActorCollider> {};

}  // namespace whal
