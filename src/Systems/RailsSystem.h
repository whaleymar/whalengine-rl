#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct RailsControl;
struct Velocity;
struct Transform;

class RailsSystem : public ecs::ISystem<RailsControl, Transform>, public ecs::IUpdate, public ecs::IMonitorSystem {
public:
    void onAdd(const ecs::Entity) override;
    void onRemove(const ecs::Entity) override {}
    void update() override;
};

}  // namespace whal
