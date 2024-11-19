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

// Exactly the same as SpriteRenderSystem::draw
void TileRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    const auto sprite = eCtx.entity.get<Sprite>();
    const auto frameSize = sprite.frameSize.as<f32>();

    const s32 flipModifier = eCtx.preciseTransform.facing == Facing::Left ? -1 : 1;
    const Rectangle srcRect = Rectangle(sprite.atlasPosition.x, sprite.atlasPosition.y, flipModifier * frameSize.x, frameSize.y);
    gfx::RaylibDrawParams params = gfx::getDrawParams(eCtx.preciseTransform, frameSize, ctx.cameraPosition);

    // If we don't deactivate, we minimize the number of shader swaps.
    // Swaps only happen if the new shader isn't the active one.
    BeginShaderMode(ShaderManager::get(Shaders::Default));
    gfx::DrawSpriteHDR(ctx.atlas.getTexture(), srcRect, params.rect, params.origin, eCtx.preciseTransform.rotationDegrees, sprite.color,
                       sprite.brightness, eCtx.colorBuf);
}

void TileRenderSystem::addToQueue(std::vector<gfx::EntityRenderInfo>& queue) const {
    for (auto [entityid, entity] : getEntitiesMutable()) {
        queue.push_back(entity.get<Tile>().renderInfo);
    }
}

void TileRenderSystem::onAdd(ecs::Entity entity) {
    const auto& trans = entity.get<Transform2D>();
    const auto bb = AABB(trans, Vector2i(PIXELS_PER_TILE, PIXELS_PER_TILE) / 2, Vector2i());

    entity.get<Tile>().renderInfo = gfx::EntityRenderInfo{
        .boundingBox = bb,
        .preciseTransform = gfx::getPreciseTrans(entity, trans),
        .entity = entity,
        .piRender = this,
    };
}

}  // namespace whal
