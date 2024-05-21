#pragma once

#include "ECS/Lib/ECS.h"

namespace whal {

struct Animator;
struct Sprite;
struct Transform;

class AnimationSystem : public ecs::ISystem<Animator, Sprite, Transform> {
public:
    void update() override;
};

}  // namespace whal
