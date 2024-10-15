#include "SpriteRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"

namespace whal {

void SpriteRenderSystem::draw(ecs::Entity entity, const RenderContext ctx) const {
    const auto sprite = entity.get<Sprite>();
    const Transform2D trans = entity.get<Transform2D>();

    Vector2f position = entity.has<PrecisePosition>() ? entity.get<PrecisePosition>().position : trans.position.as<f32>();

    const auto frameSize = sprite.getFrameSize().as<f32>();

    const s32 flipModifier = trans.facing == Facing::Left ? -1 : 1;
    const Rectangle srcRect = Rectangle(sprite.atlasPosition.x, sprite.atlasPosition.y, flipModifier * frameSize.x, frameSize.y);
    const RaylibDrawParams params = getDrawParams(position, frameSize, ctx.cameraPosition, sprite.scale, sprite.isRotateAboutCenter);
    const Color color = ctx.colorOverride ? *ctx.colorOverride : sprite.color;
    DrawTexturePro(ctx.atlas.getTexture(), srcRect, params.rect, params.origin, trans.rotationDegrees, color);
}

void SpriteRenderSystem::addToQueue(std::vector<EntityRenderInfo>& queue) const {
    queue.reserve(getEntitiesMutable().size());  // reserve space in case capacity is too low
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto sprite = entity.get<Sprite>();

        queue.emplace_back(EntityRenderInfo{
            .boundingBox = AABB(entity.get<Transform2D>(), sprite.getFrameSize() / 2),  // TODO this doesn't account for rotations
            .depth = sprite.depth,
            .entity = entity,
            .piRender = this,
        });
    }
}

}  // namespace whal
