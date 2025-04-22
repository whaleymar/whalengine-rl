#pragma once

#include "Components/Draw.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Gfx/IRender.h"
#include "whalECS/src/ECS.h"

namespace whal {

namespace gfx {
struct RenderContext;
struct EntityRenderInfo;
}  // namespace gfx

class LineRenderSystem : public ecs::ISystem<DrawStraightLine, Transform, ecs::Exclude<Invisible>>, public IRender {
public:
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(gfx::RenderQueue&) const override;
};

}  // namespace whal
