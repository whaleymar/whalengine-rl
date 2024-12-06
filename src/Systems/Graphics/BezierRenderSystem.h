#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct DrawBezierQuad;
struct Transform;
struct Invisible;

class BezierRenderSystem : public ecs::ISystem<DrawBezierQuad, Transform, ecs::Exclude<Invisible>>, public ecs::IRender {
public:
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(gfx::RenderQueue&) const override;
};

}  // namespace whal
