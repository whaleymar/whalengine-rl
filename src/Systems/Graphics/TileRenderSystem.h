#pragma once

#include "ECS.h"
#include "Events/Events.h"
#include "Sys/IListen.h"

namespace whal {

struct Sprite;
struct Transform;
struct Tile;
struct TileMapLayer;
struct Invisible;

class TileRenderSystem : public ecs::ISystem<Sprite, Transform, Tile, ecs::Exclude<Invisible>>,
                         public ecs::IRender,
                         public ecs::IMonitorSystem,
                         public IListen<evt::Restart, true, bool> {
public:
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(gfx::RenderQueue&) const override;
    void onAdd(ecs::Entity entity) override;
    void onRemove(ecs::Entity entity) override {}
    void onEvent(evt::Restart, bool) override;
};

class TileRenderSystem2 : public ecs::ISystem<Transform, TileMapLayer, ecs::Exclude<Invisible>>, public ecs::IRender {
public:
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(gfx::RenderQueue&) const override;
};

}  // namespace whal
