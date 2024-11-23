#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct DrawBezierQuad;
struct Transform;

class BezierRenderSystem : public ecs::ISystem<DrawBezierQuad, Transform>, public ecs::IRender {
public:
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(std::vector<gfx::EntityRenderInfo>&) const override;
};

}  // namespace whal
