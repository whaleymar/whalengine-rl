#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct DrawStraightLine;
struct Transform2D;
struct RenderContext;
struct EntityRenderInfo;

class LineRenderSystem : public ecs::ISystem<DrawStraightLine, Transform2D>, public ecs::IRender {
public:
    void draw(ecs::Entity entity, const RenderContext ctx) const override;
    void addToQueue(std::vector<EntityRenderInfo>&) const override;
};

}  // namespace whal
