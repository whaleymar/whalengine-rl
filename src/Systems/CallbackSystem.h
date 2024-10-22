#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct OnFrameEnd;
struct CustomUpdate;
struct OnDeath;

class OnFrameEndSystem : public ecs::ISystem<OnFrameEnd>, public ecs::IUpdate {
public:
    void update() override;
};

class CustomUpdateSystem : public ecs::ISystem<CustomUpdate>, public ecs::IUpdate {
public:
    void update() override;
};

// this assumes that by removing the onDeath component it is "dying", but it might make more sense to add an actual "onDeath" function into IMonitor
// (or another interface)
class OnDeathSystem : public ecs::ISystem<OnDeath>, public ecs::IMonitorSystem {
public:
    void onAdd(ecs::Entity) override {}
    void onRemove(ecs::Entity entity) override;
};

}  // namespace whal
