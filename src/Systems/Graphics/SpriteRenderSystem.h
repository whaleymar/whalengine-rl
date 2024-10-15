#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Sprite;
struct Transform2D;
struct RenderContext;
struct EntityRenderInfo;

class SpriteRenderSystem : public ecs::ISystem<Sprite, Transform2D>, public ecs::IRender {
public:
    void draw(ecs::Entity entity, const RenderContext ctx) const override;
    void addToQueue(std::vector<EntityRenderInfo>&) const override;
    bool isPostProcessingUsed() const override { return true; }
};

}  // namespace whal
