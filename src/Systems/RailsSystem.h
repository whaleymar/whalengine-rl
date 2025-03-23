#pragma once

#include "Components/RailsControl.h"
#include "Components/Transform.h"
#include "ECS.h"

namespace whal {

class RailsSystem : public ecs::ISystem<RailsControl, Transform>, public ecs::IUpdate, public ecs::IMonitorSystem {
public:
    void onAdd(const ecs::Entity) override;
    void onRemove(const ecs::Entity) override {}
    void update() override;
};

}  // namespace whal
