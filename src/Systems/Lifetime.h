#pragma once

#include "whalECS/src/ECS.h"
namespace whal {

struct Lifetime;
struct DieWhenSpeedBelow;
struct Velocity;

class LifetimeSystem : public ecs::ISystem<Lifetime>, public ecs::IUpdate {
public:
    void update() override;
};

class SlowEntityKillerSystem : public ecs::ISystem<DieWhenSpeedBelow, Velocity>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal
