#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Sprite;
struct Transform;
struct Tile;
struct Invisible;

class TileRenderSystem : public ecs::ISystem<Sprite, Transform, Tile, ecs::Exclude<Invisible>>, public ecs::IRender, public ecs::IMonitorSystem {
public:
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(std::vector<gfx::EntityRenderInfo>&) const override;
    void onAdd(ecs::Entity entity) override;
    void onRemove(ecs::Entity entity) override {}
};

}  // namespace whal
