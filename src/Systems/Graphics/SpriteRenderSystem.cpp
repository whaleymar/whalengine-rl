#include "SpriteRenderSystem.h"

#include "Common.h"
#include "Components/Collision.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/Texture.h"
#include "Physics/Box.h"

#include "rlgl.h"

namespace whal {

void SpriteRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    const auto& sprite = eCtx.entity.get<Sprite>();

    const s32 flipModifier = eCtx.transform.facing == Facing::Left ? -1 : 1;
    const rl::Rectangle srcRect =
        rl::Rectangle{sprite.atlasPosition.x, sprite.atlasPosition.y, flipModifier * sprite.frameSize.x, sprite.frameSize.y};
    const gfx::RaylibDrawParams params = gfx::getDrawParams(eCtx.transform, sprite.frameSize);

    gfx::DrawSpriteHDR(ctx.atlas.getTexture(), srcRect, params.rect, params.origin, eCtx.transform.rotation, sprite.color.asRL(),
                       eCtx.colorBuf.asRL(sprite, ctx.atlas.getSize()));
}

void SpriteRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    for (const auto& [entityid, entity] : getEntities()) {
        const auto sprite = entity.get<Sprite>();
        const auto& trans = entity.get<Transform>();
        const auto bb = trans.rotation == 0.0f ?
                            AABB(trans, sprite.frameSize.as<s32>() / 2, Vector2i()) :
                            Box(trans.getRotatedPosition().round(), sprite.frameSize.as<s32>() / 2, trans.rotation).getBoundingAABB();

        // make physics objects appear to move smoothly
        if (entity.has<Collider>()) {
            queue.add(gfx::EntityPreRenderInfo{
                .boundingBox = bb,
                // .transform = TransformBuilder(trans).translate(entity.get<Collider>().getRemainder()).build(),
                .transform = trans,
                .entity = entity,
            });
        } else {
            queue.add(gfx::EntityPreRenderInfo{
                .boundingBox = bb,
                .transform = trans,
                .entity = entity,
            });
        }
    }
}

}  // namespace whal
