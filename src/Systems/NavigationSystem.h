#pragma once

// just for debugging (for now)
#ifndef NDEBUG
#include "Sys/IListen.h"
#include "Util/Types.h"
#include "whalECS/src/ECS.h"

namespace whal {

struct Path;
struct Transform;
enum class InputType : u64;

namespace evt {
class ButtonPress;
}

class NavigationSystem : public ecs::ISystem<Transform, Path>, public ecs::IRender, public IListen<evt::ButtonPress, false, InputType> {
public:
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(std::vector<gfx::EntityRenderInfo>& queue) const override;
    void onEvent(evt::ButtonPress, InputType input) override;
};

}  // namespace whal
#endif
