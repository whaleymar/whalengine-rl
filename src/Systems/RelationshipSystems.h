#pragma once

#include "Components/Relationships.h"
#include "Components/Transform.h"
#include "Components/Velocity.h"
#include "Events/Events.h"
#include "Sys/IListen.h"
#include "whalECS/src/ECS.h"

namespace whal {

class FollowSystem : public ecs::ISystem<Follow, Velocity, Transform>,
                     public ecs::IUpdate,
                     public ecs::IMonitorSystem,
                     public IListen<evt::Death, true, ecs::Entity> {
public:
    void update() override;
    void onAdd(ecs::Entity entity) override {}
    void onRemove(ecs::Entity entity) override;
    void onEvent(evt::Death, ecs::Entity entity) override;
};

}  // namespace whal
