#pragma once

#include "ECS.h"
#include "Util/Types.h"

namespace whal {

struct Transform;
struct TileMapLayer;
struct Invisible;

struct TileInstance {
    u32 tileMask;
    s32 x;
    s32 y;
};

class TileRenderSystem : public ecs::ISystem<Transform, TileMapLayer, ecs::Exclude<Invisible>>, public ecs::IRender, public ecs::IMonitorSystem {
public:
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(gfx::RenderQueue&) const override;
    void onAdd(ecs::Entity) override;
    void onRemove(ecs::Entity e) override;

private:
    mutable std::vector<TileInstance> mDrawQueue;
};

}  // namespace whal
