#include "RectangleRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"

namespace whal {

void RectangleRenderSystem::draw(ecs::Entity entity, const RenderContext ctx) const {
    const DrawRect rect = entity.get<DrawRect>();
    const Transform2D trans = entity.get<Transform2D>();

    Vector2f position = entity.has<PrecisePosition>() ? entity.get<PrecisePosition>().position : trans.position.as<f32>();

    const auto frameSize = rect.getFrameSize().as<f32>();
    // TODO these should have rotation enabled
    const RaylibDrawParams params = getDrawParams(position, frameSize, ctx.cameraPosition, rect.scale, false);
    const Color color = ctx.colorOverride ? *ctx.colorOverride : rect.color;
    DrawRectanglePro(params.rect, params.origin, 0.0f, color);
}

void RectangleRenderSystem::addToQueue(std::vector<EntityRenderInfo>& queue) const {
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto draw = entity.get<DrawRect>();

        queue.emplace_back(EntityRenderInfo{
            .boundingBox = AABB(entity.get<Transform2D>(), draw.getFrameSize() / 2),
            .depth = draw.depth,
            .entity = entity,
            .piRender = this,
        });
    }
}

}  // namespace whal
