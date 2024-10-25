#pragma once

#include "whalECS/src/ECS.h"
namespace whal {

struct MouseCursor;
struct Transform2D;
struct Sprite;

class MouseCursorSystem : public ecs::ISystem<MouseCursor, Transform2D, Sprite>, public ecs::AttrUniqueEntity, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal
