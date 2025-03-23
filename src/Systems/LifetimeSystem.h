#pragma once

#include "Components/Lifetime.h"
#include "whalECS/src/ECS.h"

namespace whal {

class LifetimeSystem : public ecs::ISystem<Lifetime>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal
