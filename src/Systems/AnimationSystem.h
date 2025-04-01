#pragma once

#include "Components/Animator.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "whalECS/src/ECS.h"

namespace whal {

class AnimationSystem : public ecs::ISystem<Animator, Transform, ecs::Exclude<Invisible>>,
                        public ecs::IUpdate /*,public ecs::AttrUpdateDuringPause*/,
                        public ecs::IMonitorSystem {
public:
    void update() override;
    void onAdd(ecs::Entity e) override;
    void onRemove(ecs::Entity e) override {}
};

}  // namespace whal
