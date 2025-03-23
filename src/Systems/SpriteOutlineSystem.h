#pragma once

#include "Components/Draw.h"
#include "Components/Transform.h"
#include "ECS.h"

namespace whal {

// NOTE: for correct results, this should run after the AnimationSystem
class SpriteOutlineSystem : public ecs::ISystem<Transform, Sprite, SpriteOutline>, public ecs::IUpdate, public ecs::IMonitorSystem {
public:
    void onAdd(ecs::Entity entity) override;
    void onRemove(ecs::Entity entity) override {}
    void update() override;
};

}  // namespace whal
