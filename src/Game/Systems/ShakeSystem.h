#pragma once

#include "whalECS/src/ECS.h"

namespace whal {
struct Transform2D;
}  // namespace whal

struct Shake;

class ShakeSystem : public whal::ecs::ISystem<whal::Transform2D, Shake>, public whal::ecs::IUpdate, public whal::ecs::IMonitorSystem {
public:
    void onAdd(whal::ecs::Entity) override;
    void onRemove(whal::ecs::Entity) override {}  // RESEARCH would be safe to cancel the tween if shake is manually removed
    void update() override;
};
