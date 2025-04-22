#pragma once

#include "Components/Draw.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Gfx/IRender.h"
#include "whalECS/src/ECS.h"

namespace whal {

class BezierRenderSystem : public ecs::ISystem<DrawBezierQuad, Transform, ecs::Exclude<Invisible>>, public IRender {
public:
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(gfx::RenderQueue&) const override;
};

}  // namespace whal
