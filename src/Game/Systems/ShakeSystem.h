#pragma once

#include "whalECS/src/ECS.h"

namespace whal {
struct Transform2D;
class Draw;
}  // namespace whal

struct Shake;

class ShakeSystem : public whal::ecs::ISystem<whal::Transform2D, whal::Draw, Shake>, public whal::ecs::IUpdate, public whal::ecs::IMonitorSystem {
public:
    void onAdd(whal::ecs::Entity) override;
    void onRemove(whal::ecs::Entity) override {}
    void update() override;
};
