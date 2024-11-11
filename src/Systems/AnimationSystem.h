#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Animator;
struct Sprite;
struct Transform2D;

class AnimationSystem : public ecs::ISystem<Animator, Sprite, Transform2D>, public ecs::IUpdate /*,public ecs::AttrUpdateDuringPause*/ {
public:
    void update() override;
};

}  // namespace whal
