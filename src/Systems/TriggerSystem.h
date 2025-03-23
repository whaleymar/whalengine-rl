#pragma once

#include "Components/Transform.h"
#include "Components/TriggerZone.h"
#include "whalECS/src/ECS.h"

namespace whal {

class TriggerSystem : public ecs::ISystem<Transform, Trigger>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal
