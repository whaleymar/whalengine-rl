#include "SpriteRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Physics/Box.h"

namespace whal {

void SpriteRenderSystem::draw(ecs::Entity entity, const RenderContext ctx) const {
    const auto sprite = entity.get<Sprite>();
    const Transform2D trans = entity.get<Transform2D>();

    PreciseTransform2D pTrans = PreciseTransform2D::fromTrans(trans);
    if (entity.has<PreciseTransform2D>()) {
        pTrans.position = entity.get<PrecisePosition>().position;
    }

    const auto frameSize = sprite.getFrameSize().as<f32>();

    const s32 flipModifier = trans.facing == Facing::Left ? -1 : 1;
    const Rectangle srcRect = Rectangle(sprite.atlasPosition.x, sprite.atlasPosition.y, flipModifier * frameSize.x, frameSize.y);
    RaylibDrawParams params = getDrawParamsNew(pTrans, frameSize, ctx.cameraPosition);
    const Color color = ctx.colorOverride ? *ctx.colorOverride : sprite.color;
    DrawTexturePro(ctx.atlas.getTexture(), srcRect, params.rect, params.origin, trans.rotationDegrees, color);
}

void SpriteRenderSystem::addToQueue(std::vector<EntityRenderInfo>& queue) const {
    queue.reserve(getEntitiesMutable().size());  // reserve space in case capacity is too low
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto sprite = entity.get<Sprite>();
        const auto trans = entity.get<Transform2D>();
        const auto bb = trans.rotationDegrees == 0.0f ?
                            AABB(entity.get<Transform2D>(), sprite.getFrameSize() / 2) :
                            Box(trans.getRotatedPosition(), sprite.getFrameSize() / 2, trans.rotationDegrees).getBoundingAABB();

        queue.emplace_back(EntityRenderInfo{
            .boundingBox = bb,
            .depth = sprite.depth,
            .entity = entity,
            .piRender = this,
        });
    }
}

}  // namespace whal
