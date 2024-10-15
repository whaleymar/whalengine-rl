#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct DrawRect;
struct Transform2D;
struct RenderContext;
struct EntityRenderInfo;

class RectangleRenderSystem : public ecs::ISystem<DrawRect, Transform2D>, public ecs::IRender {
public:
    void draw(ecs::Entity entity, const RenderContext ctx) const override;
    void addToQueue(std::vector<EntityRenderInfo>&) const override;
    bool isPostProcessingUsed() const override { return true; }
};

}  // namespace whal
