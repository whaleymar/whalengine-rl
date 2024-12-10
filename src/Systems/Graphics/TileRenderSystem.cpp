#include "TileRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Tile.h"
#include "Components/Transform.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Physics/Box.h"

#include "Settings.h"
#include "rlgl.h"

namespace whal {

void TileRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    const auto& sprite = eCtx.entity.get<Sprite>();
    const auto frameSize = sprite.frameSize.as<f32>();

    const s32 flipModifier = eCtx.preciseTransform.facing == Facing::Left ? -1 : 1;
    const auto srcRect = rl::Rectangle(sprite.atlasPosition.x, sprite.atlasPosition.y, flipModifier * frameSize.x, frameSize.y);

    // simplified version of gfx::getDrawParams that assumes no fancy rotation or scaling
    static const Vector2f size = Vector2f(PIXELS_PER_TILE, PIXELS_PER_TILE) * VIRTUAL_SCREEN_RATIO;
    static const rl::Vector2 origin = (size * Vector2f(0.5, 0.5)).asRL();
    const rl::Rectangle rect = rl::Rectangle{
        (eCtx.preciseTransform.position.x - ctx.cameraPosition.x) * VIRTUAL_SCREEN_RATIO + FWINDOW_WIDTH_RENDER / 2,
        (ctx.cameraPosition.y - eCtx.preciseTransform.position.y) * VIRTUAL_SCREEN_RATIO + FWINDOW_HEIGHT_RENDER / 2,
        size.x,
        size.y,
    };

    auto meta = eCtx.colorBuf;
    meta.setFlags(sprite, ctx.atlas.getSize());

    // If we don't deactivate, we minimize the number of shader swaps.
    // Swaps only happen if the new shader isn't the active one.
    BeginShaderMode(ShaderManager::get(Shaders::Default));
    gfx::DrawSpriteHDR(ctx.atlas.getTexture(), srcRect, rect, origin, eCtx.preciseTransform.rotationDegrees, sprite.color, meta);
}

void TileRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    for (auto [entityid, entity] : getEntitiesMutable()) {
        queue.push_back(entity.get<Tile>().renderInfo);
    }
}

void TileRenderSystem::onAdd(ecs::Entity entity) {
    const auto& trans = entity.get<Transform>();
    const auto bb = AABB(trans, Vector2i(PIXELS_PER_TILE, PIXELS_PER_TILE) / 2, Vector2i());

    entity.get<Tile>().renderInfo = gfx::EntityRenderLoc{
        .boundingBox = bb,
        .preciseTransform = gfx::getPreciseTrans(entity, trans),
        .entity = entity,
    };
}

}  // namespace whal
