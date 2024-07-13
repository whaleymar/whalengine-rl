#pragma once

#include "whalECS/src/ECS.h"
namespace whal {

struct Lifetime;

class LifetimeSystem : public ecs::ISystem<Lifetime>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal
