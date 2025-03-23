#pragma once

#include "Components/Animator.h"
#include "Components/Draw.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "whalECS/src/ECS.h"

namespace whal {

class AnimationSystem : public ecs::ISystem<Animator, Sprite, Transform, ecs::Exclude<Invisible>>,
                        public ecs::IUpdate /*,public ecs::AttrUpdateDuringPause*/ {
public:
    void update() override;
};

}  // namespace whal
