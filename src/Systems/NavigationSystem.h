#pragma once

// just for debugging (for now)
#ifndef NDEBUG
#include "whalECS/src/ECS.h"

namespace whal {

struct Path;
struct Transform2D;
enum class InputType : u64;
class ButtonPressEvent;

class NavigationSystem : public ecs::ISystem<Transform2D, Path>, public ecs::IRender, public IListen<ButtonPressEvent, false, InputType> {
public:
    void draw(ecs::Entity entity, const RenderContext ctx) const override;
    void addToQueue(std::vector<EntityRenderInfo>& queue) const override;
    void onEvent(ButtonPressEvent, InputType input) override;
};

}  // namespace whal
#endif
