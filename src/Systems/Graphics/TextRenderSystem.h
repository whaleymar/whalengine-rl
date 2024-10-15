#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct DrawText;
struct Transform2D;
struct RenderContext;
struct EntityRenderInfo;

class TextRenderSystem : public ecs::ISystem<DrawText, Transform2D>, public ecs::IRender {
public:
    TextRenderSystem();
    void draw(ecs::Entity entity, const RenderContext ctx) const override;
    void addToQueue(std::vector<EntityRenderInfo>&) const override;
};

}  // namespace whal
