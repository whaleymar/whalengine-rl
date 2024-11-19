#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct DrawRect;
struct Transform2D;

class RectangleRenderSystem : public ecs::ISystem<DrawRect, Transform2D>, public ecs::IRender {
public:
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(std::vector<gfx::EntityRenderInfo>&) const override;
};

}  // namespace whal
