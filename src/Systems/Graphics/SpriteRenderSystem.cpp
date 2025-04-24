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

static bool isChildDrawable(ecs::Entity child, const ecs::SystemBase* system);
static void tryDrawBehind(ecs::Entity child, const gfx::RenderContext& ctx, const ecs::SystemBase* system);
static void tryDrawOnTop(ecs::Entity child, const gfx::RenderContext& ctx, const ecs::SystemBase* system);
static void drawEntityAndChildren(ecs::Entity e, const gfx::RenderContext& ctx, const ecs::SystemBase* system);

void SpriteRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    // TODO I will need to bring the Renderer's shader state tracker here. Right now the parent shader applies to all children
    drawEntityAndChildren(eCtx.entity, ctx, this);
}

void SpriteRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    for (const auto [entityid, entity] : getEntities()) {
        entity.getTrait<IDrawable>().queue(entity, queue);
    }
}

void drawSprite(ecs::Entity child, const gfx::RenderContext& ctx) {
    const Transform& transform = child.get<Transform>();

    const Sprite& sprite = child.get<Sprite>();
    const rl::Rectangle srcRect = rl::Rectangle{sprite.atlasPosition.x, sprite.atlasPosition.y, math::sign(transform.scale.x) * sprite.frameSize.x,
                                                math::sign(transform.scale.y) * sprite.frameSize.y};
    const gfx::RaylibDrawParams params = gfx::getDrawParams(transform, sprite.frameSize);

    bool isUI = transform.depth == Depth::Debug || transform.depth == Depth::UIFar || transform.depth == Depth::UIClose;
    gfx::DrawSpriteHDR(ctx.atlas.getTexture(), srcRect, params.rect, params.origin, transform.rotation, sprite.color.asRL(),
                       gfx::DrawMetaData{.depth = static_cast<u8>(transform.depth), .isUI = isUI}.asRL(sprite, ctx.atlas.getSize()), sprite.custom0b);
}

void queueSprite(ecs::Entity entity, gfx::RenderQueue& queue) {
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

void drawRectangle(ecs::Entity entity, const gfx::RenderContext& ctx) {
    const DrawRect& rect = entity.get<DrawRect>();
    const Transform& trans = entity.get<Transform>();

    const auto frameSize = rect.frameSize.as<f32>();
    const gfx::RaylibDrawParams params = gfx::getDrawParams(trans, frameSize);

    bool isUI = trans.depth == Depth::Debug || trans.depth == Depth::UIFar || trans.depth == Depth::UIClose;
    gfx::DrawRectangleHDR(params.rect, params.origin, trans.rotation, rect.color,
                          gfx::DrawMetaData{.depth = static_cast<u8>(trans.depth), .isUI = isUI});
}

void queueRectangle(ecs::Entity entity, gfx::RenderQueue& queue) {
    const DrawRect& draw = entity.get<DrawRect>();
    const Transform& trans = entity.get<Transform>();
    AABB bb = trans.rotation == 0.0f ? AABB(trans, draw.frameSize / 2) :
                                       OBB(trans.getRotatedPosition().round(), draw.frameSize / 2, trans.rotation).getBoundingAABB();

    // float height should affect bounding box (for culling) but not Y sorting
    s32 floatOffset = static_cast<s32>(trans.z * FLOAT_HEIGHT_MULT);
    bb.getPositionMut().y += floatOffset;
    queue.add(gfx::EntityPreRenderInfo{
        .boundingBox = bb,
        .transform = &trans,
        .ysortPosition = bb.bottom() - floatOffset,
        .entity = entity,
    });
}

// TODO do I need to call invisible here? isMatch should cover that
bool isChildDrawable(ecs::Entity child, const ecs::SystemBase* system) {
    return !SpriteRenderSystem::getEntities().contains(child.id()) && !child.has<Invisible>() && system->isMatch(child);
}

void tryDrawBehind(ecs::Entity child, const gfx::RenderContext& ctx, const ecs::SystemBase* system) {
    if (!child.has<DrawOnTopOfParent>() && isChildDrawable(child, system)) {
        drawEntityAndChildren(child, ctx, system);
    }
}

void tryDrawOnTop(ecs::Entity child, const gfx::RenderContext& ctx, const ecs::SystemBase* system) {
    if (child.has<DrawOnTopOfParent>() && isChildDrawable(child, system)) {
        drawEntityAndChildren(child, ctx, system);
    }
}

void drawEntityAndChildren(ecs::Entity e, const gfx::RenderContext& ctx, const ecs::SystemBase* system) {
    e.forChild(tryDrawBehind, false, ctx, system);
    e.getTrait<IDrawable>().draw(e, ctx);
    e.forChild(tryDrawOnTop, false, ctx, system);
}

}  // namespace whal
