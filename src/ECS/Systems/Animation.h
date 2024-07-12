#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Animator;
class Draw;
struct Transform2D;

class AnimationSystem : public ecs::ISystem<Animator, Draw, Transform2D>, public ecs::IUpdate, public ecs::AttrUpdateDuringPause {
public:
    void update() override;
};

}  // namespace whal
