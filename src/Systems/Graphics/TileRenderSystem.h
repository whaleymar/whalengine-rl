#pragma once

#include "ECS.h"

namespace whal {

struct Transform;
struct TileMapLayer;
struct Invisible;

class TileRenderSystem : public ecs::ISystem<Transform, TileMapLayer, ecs::Exclude<Invisible>>, public ecs::IRender, public ecs::IMonitorSystem {
public:
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(gfx::RenderQueue&) const override;
    void onAdd(ecs::Entity) override {}
    void onRemove(ecs::Entity e) override;
};

}  // namespace whal
