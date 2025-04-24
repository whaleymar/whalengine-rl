#pragma once

namespace whal {

namespace gfx {
// struct EntityRenderInfo;
struct RenderContext;
class RenderQueue;
}  // namespace gfx

namespace ecs {
class Entity;
}

// Trait for drawable components
struct IDrawable {
    using DrawMethod = void (*)(ecs::Entity, const gfx::RenderContext& ctx);
    using QueueMethod = void (*)(ecs::Entity, gfx::RenderQueue& queue);
    DrawMethod draw = nullptr;
    QueueMethod queue = nullptr;
};

}  // namespace whal
