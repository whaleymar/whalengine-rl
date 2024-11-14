#include "SpriteRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Gfx/Texture.h"
#include "Physics/Box.h"

namespace whal {

void SpriteRenderSystem::draw(ecs::Entity entity, const gfx::RenderContext ctx) const {
    const auto sprite = entity.get<Sprite>();
    const PreciseTransform2D trans = gfx::getPreciseTrans(entity);
    const auto frameSize = sprite.frameSize.as<f32>();

    const s32 flipModifier = trans.facing == Facing::Left ? -1 : 1;
    const Rectangle srcRect = Rectangle(sprite.atlasPosition.x, sprite.atlasPosition.y, flipModifier * frameSize.x, frameSize.y);
    gfx::RaylibDrawParams params = gfx::getDrawParamsNew(trans, frameSize, ctx.cameraPosition);
    const Color color = ctx.colorOverride ? *ctx.colorOverride : sprite.color;
    DrawTexturePro(ctx.atlas.getTexture(), srcRect, params.rect, params.origin, trans.rotationDegrees, color);
}

void SpriteRenderSystem::addToQueue(std::vector<gfx::EntityRenderInfo>& queue) const {
    queue.reserve(getEntitiesMutable().size());  // reserve space in case capacity is too low
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto sprite = entity.get<Sprite>();
        const auto trans = entity.get<Transform2D>();
        const auto bb = trans.rotationDegrees == 0.0f ?
                            AABB(trans, sprite.frameSize / 2, Vector2i()) :
                            Box(trans.getRotatedPosition(), sprite.frameSize / 2, trans.rotationDegrees).getBoundingAABB();

        queue.emplace_back(gfx::EntityRenderInfo{
            .boundingBox = bb,
            .depth = trans.depth,
            .entity = entity,
            .piRender = this,
        });
    }
}

}  // namespace whal
