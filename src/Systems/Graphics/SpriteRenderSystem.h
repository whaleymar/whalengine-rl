#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Sprite;
struct Transform2D;

class SpriteRenderSystem : public ecs::ISystem<Sprite, Transform2D>, public ecs::IRender {
public:
    void draw(ecs::Entity entity, const gfx::RenderContext ctx) const override;
    void addToQueue(std::vector<gfx::EntityRenderInfo>&) const override;
    bool isPostProcessingUsed() const override { return true; }
};

}  // namespace whal
