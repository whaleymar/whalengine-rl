#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct TweenPosition;
struct Transform2D;
struct Velocity;

// Tween movement for entities that are *not* in the physics system. Physics objecst should use RailsControl
class TweenPositionSystem : public ecs::ISystem<TweenPosition, Transform2D, ecs::Lacks<Velocity>>,
                            public ecs::IUpdate,
                            public ecs::IMonitorSystem,
                            public ecs::AttrUpdateDuringPause {
    void update() override;
    void onAdd(ecs::Entity entity) override;
    void onRemove(ecs::Entity) override {}
};

}  // namespace whal
