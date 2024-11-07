#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct DrawStraightLine;
struct Transform2D;

namespace gfx {
struct RenderContext;
struct EntityRenderInfo;
}  // namespace gfx

class LineRenderSystem : public ecs::ISystem<DrawStraightLine, Transform2D>, public ecs::IRender {
public:
    void draw(ecs::Entity entity, const gfx::RenderContext ctx) const override;
    void addToQueue(std::vector<gfx::EntityRenderInfo>&) const override;
};

}  // namespace whal
