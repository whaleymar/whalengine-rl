#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Sprite;
struct Transform;
struct Tile;
struct Invisible;

class SpriteRenderSystem : public ecs::ISystem<Sprite, Transform, ecs::Exclude<Tile>, ecs::Exclude<Invisible>>, public ecs::IRender {
public:
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(gfx::RenderQueue&) const override;
};

}  // namespace whal
