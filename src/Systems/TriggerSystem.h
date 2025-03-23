#pragma once

#include "Components/Transform.h"
#include "Components/Trigger.h"
#include "whalECS/src/ECS.h"

namespace whal {

class TriggerSystem : public ecs::ISystem<Transform, Trigger>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal
