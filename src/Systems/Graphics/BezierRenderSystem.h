#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct DrawBezierQuad;
struct Transform2D;

class BezierRenderSystem : public ecs::ISystem<DrawBezierQuad, Transform2D>, public ecs::IRender {
public:
    void draw(ecs::Entity entity, const gfx::RenderContext ctx) const override;
    void addToQueue(std::vector<gfx::EntityRenderInfo>&) const override;
};

}  // namespace whal
