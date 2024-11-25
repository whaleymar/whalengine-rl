#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct DrawStraightLine;
struct Transform;
struct Invisible;

namespace gfx {
struct RenderContext;
struct EntityRenderInfo;
}  // namespace gfx

class LineRenderSystem : public ecs::ISystem<DrawStraightLine, Transform, ecs::Exclude<Invisible>>, public ecs::IRender {
public:
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(std::vector<gfx::EntityRenderInfo>&) const override;
};

}  // namespace whal
