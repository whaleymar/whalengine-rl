#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Animator;
struct Sprite;
struct Transform;
struct Invisible;

class AnimationSystem : public ecs::ISystem<Animator, Sprite, Transform, ecs::Exclude<Invisible>>,
                        public ecs::IUpdate /*,public ecs::AttrUpdateDuringPause*/ {
public:
    void update() override;
};

}  // namespace whal
