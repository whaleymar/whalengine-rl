#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Animator;
struct Sprite;
struct Transform;

class AnimationSystem : public ecs::ISystem<Animator, Sprite, Transform>, public ecs::IUpdate /*,public ecs::AttrUpdateDuringPause*/ {
public:
    void update() override;
};

}  // namespace whal
