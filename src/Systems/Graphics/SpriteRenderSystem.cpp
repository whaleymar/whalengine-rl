#include "SpriteRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Physics/Box.h"

#include "rlgl.h"

namespace whal {

void SpriteRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    const auto& sprite = eCtx.entity.get<Sprite>();
    const auto frameSize = sprite.frameSize.as<f32>();

    const s32 flipModifier = eCtx.preciseTransform.facing == Facing::Left ? -1 : 1;
    const rl::Rectangle srcRect = rl::Rectangle(sprite.atlasPosition.x, sprite.atlasPosition.y, flipModifier * frameSize.x, frameSize.y);
    gfx::RaylibDrawParams params = gfx::getDrawParams(eCtx.preciseTransform, frameSize, ctx.cameraPosition);

    auto meta = eCtx.colorBuf;
    meta.setFlags(sprite, ctx.atlas.getSize());

    // If we don't deactivate, we minimize the number of shader swaps.
    // Swaps only happen if the new shader isn't the active one.
    rl::BeginShaderMode(ShaderManager::get(Shaders::Default));
    gfx::DrawSpriteHDR(ctx.atlas.getTexture(), srcRect, params.rect, params.origin, eCtx.preciseTransform.rotationDegrees, sprite.color, meta);
}

void SpriteRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto sprite = entity.get<Sprite>();
        const auto& trans = entity.get<Transform>();
        const auto bb = trans.rotationDegrees == 0.0f ?
                            AABB(trans, sprite.frameSize / 2, Vector2i()) :
                            Box(trans.getRotatedPosition(), sprite.frameSize / 2, trans.rotationDegrees).getBoundingAABB();

        queue.push_back(gfx::EntityRenderLoc{
            .boundingBox = bb,
            .preciseTransform = gfx::getPreciseTrans(entity, trans),
            .entity = entity,
        });
    }
}

}  // namespace whal
