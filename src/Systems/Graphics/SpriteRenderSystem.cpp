#include "SpriteRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/Texture.h"
#include "Physics/Box.h"

#include "Settings.h"
#include "rlgl.h"

namespace whal {

void SpriteRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    const auto& sprite = eCtx.entity.get<Sprite>();

    const rl::Rectangle srcRect =
        rl::Rectangle{sprite.atlasPosition.x, sprite.atlasPosition.y, math::sign(eCtx.transform->scale.x) * sprite.frameSize.x,
                      math::sign(eCtx.transform->scale.y) * sprite.frameSize.y};
    const gfx::RaylibDrawParams params = gfx::getDrawParams(*eCtx.transform, sprite.frameSize);

    gfx::DrawSpriteHDR(ctx.atlas.getTexture(), srcRect, params.rect, params.origin, eCtx.transform->rotation, sprite.color.asRL(),
                       eCtx.colorBuf.asRL(sprite, ctx.atlas.getSize()));
}

void SpriteRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    for (const auto& [entityid, entity] : getEntities()) {
        const Sprite& sprite = entity.get<Sprite>();
        const Transform& trans = entity.get<Transform>();
        AABB bb = trans.rotation == 0.0f ? AABB(trans, sprite.frameSize.as<s32>() / 2, Vector2i()) :
                                           Box(trans.getRotatedPosition().round(), sprite.frameSize.as<s32>() / 2, trans.rotation).getBoundingAABB();

        // float height should affect bounding box (for culling) but not Y sorting
        s32 floatOffset = static_cast<s32>(trans.floatHeight * FLOAT_HEIGHT_MULT);
        bb.getPositionMut().y += floatOffset;
        queue.add(gfx::EntityPreRenderInfo{
            .boundingBox = bb,
            .transform = &trans,
            .ysortPosition = bb.bottom() - floatOffset,
            .entity = entity,
            .shader = sprite.shader,
        });
    }
}

}  // namespace whal
