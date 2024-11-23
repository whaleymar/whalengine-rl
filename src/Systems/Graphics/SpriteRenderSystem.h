#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Sprite;
struct Transform;
struct Tile;

class SpriteRenderSystem : public ecs::ISystem<Sprite, Transform, ecs::Exclude<Tile>>, public ecs::IRender {
public:
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(std::vector<gfx::EntityRenderInfo>&) const override;
};

}  // namespace whal
