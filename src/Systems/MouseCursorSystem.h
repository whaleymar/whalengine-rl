#pragma once

#include "whalECS/src/ECS.h"
namespace whal {

struct MouseCursor;
struct Transform;
struct Sprite;

class MouseCursorSystem : public ecs::ISystem<MouseCursor, Transform, Sprite>, public ecs::AttrUniqueEntity, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal
