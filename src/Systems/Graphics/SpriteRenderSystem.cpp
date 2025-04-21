#include "SpriteRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/Texture.h"
#include "Physics/OBB.h"

#include "Settings.h"

namespace whal {

static void drawSprite(const Sprite& sprite, const Transform& transform, gfx::DrawMetaData colorBuf, const gfx::RenderContext& ctx) {
    const rl::Rectangle srcRect = rl::Rectangle{sprite.atlasPosition.x, sprite.atlasPosition.y, math::sign(transform.scale.x) * sprite.frameSize.x,
                                                math::sign(transform.scale.y) * sprite.frameSize.y};
    const gfx::RaylibDrawParams params = gfx::getDrawParams(transform, sprite.frameSize);

    gfx::DrawSpriteHDR(ctx.atlas.getTexture(), srcRect, params.rect, params.origin, transform.rotation, sprite.color.asRL(),
                       colorBuf.asRL(sprite, ctx.atlas.getSize()), sprite.custom0b);
}

static void drawEntityAndChildren(ecs::Entity e, const gfx::RenderContext& ctx, const ecs::SystemBase* system);
static void drawEntity(ecs::Entity child, const gfx::RenderContext& ctx) {
    const Transform& trans = child.get<Transform>();
    bool isUI = trans.depth == Depth::Debug || trans.depth == Depth::UIFar || trans.depth == Depth::UIClose;

    drawSprite(child.get<Sprite>(), trans, gfx::DrawMetaData{.depth = static_cast<u8>(trans.depth), .isUI = isUI}, ctx);
}

static bool isChildDrawable(ecs::Entity child, const ecs::SystemBase* system) {
    return !SpriteRenderSystem::getEntities().contains(child.id()) && !child.has<Invisible>() && system->isMatch(child);
}

static void tryDrawBehind(ecs::Entity child, const gfx::RenderContext& ctx, const ecs::SystemBase* system) {
    if (!child.has<DrawOnTopOfParent>() && isChildDrawable(child, system)) {
        drawEntityAndChildren(child, ctx, system);
    }
}

static void tryDrawOnTop(ecs::Entity child, const gfx::RenderContext& ctx, const ecs::SystemBase* system) {
    if (child.has<DrawOnTopOfParent>() && isChildDrawable(child, system)) {
        drawEntityAndChildren(child, ctx, system);
    }
}

static void drawEntityAndChildren(ecs::Entity e, const gfx::RenderContext& ctx, const ecs::SystemBase* system) {
    e.forChild(tryDrawBehind, false, ctx, system);
    drawEntity(e, ctx);
    e.forChild(tryDrawOnTop, false, ctx, system);
}

void SpriteRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    // RESEARCH: can I make this work across different render systems?
    drawEntityAndChildren(eCtx.entity, ctx, this);
}

void SpriteRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    for (const auto& [entityid, entity] : getEntities()) {
        const Sprite& sprite = entity.get<Sprite>();
        const Transform& trans = entity.get<Transform>();
        AABB bb = trans.rotation == 0.0f ? AABB(trans, sprite.frameSize.as<s32>() / 2, Vector2i()) :
                                           OBB(trans.getRotatedPosition().round(), sprite.frameSize.as<s32>() / 2, trans.rotation).getBoundingAABB();

        // float height should affect bounding box (for culling) but not Y sorting
        s32 floatOffset = static_cast<s32>(trans.z * FLOAT_HEIGHT_MULT);
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
