#pragma once

#include "Components/Draw.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "whalECS/src/ECS.h"
namespace whal {

class MouseCursorSystem : public ecs::ISystem<MouseCursor, Transform, Sprite>,
                          public ecs::AttrUniqueEntity,
                          public ecs::AttrUpdateDuringPause,
                          public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal
