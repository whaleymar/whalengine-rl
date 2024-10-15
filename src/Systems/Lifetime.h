#pragma once

#include "whalECS/src/ECS.h"
namespace whal {

struct Lifetime;
struct Velocity;

class LifetimeSystem : public ecs::ISystem<Lifetime>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal
