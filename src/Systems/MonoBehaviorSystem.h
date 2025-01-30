#pragma once

#include "ECS.h"
#include "Events/Events.h"
#include "Sys/IListen.h"

namespace whal {

struct MonoBehavior;

class MonoBehaviorSystem : public ecs::ISystem<MonoBehavior>,
                           public ecs::IUpdate,
                           IListen<evt::Input, false, InputEvent>,
                           public ecs::IMonitorSystem {
public:
    void update() override;
    void onAdd(ecs::Entity entity) override;
    void onRemove(ecs::Entity entity) override;

    void onEvent(evt::Input, InputEvent input) override;
};

}  // namespace whal
