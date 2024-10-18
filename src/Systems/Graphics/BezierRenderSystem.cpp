#include "BezierRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Settings.h"

namespace whal {

// NOTE rotation not implemented
void BezierRenderSystem::draw(ecs::Entity entity, const RenderContext ctx) const {
    const auto bezier = entity.get<DrawBezierQuad>();
    const Transform2D trans = entity.get<Transform2D>();
    const Vector2f position = entity.has<PrecisePosition>() ? entity.get<PrecisePosition>().position : trans.position.as<f32>();

    Vector2 p1 = getDrawParams(position, Vector2f::zero, ctx.cameraPosition, Vector2f::zero, false).position;
    Vector2 controlPoint =
        getDrawParams(position + bezier.controlPointOffset.as<f32>(), Vector2f::zero, ctx.cameraPosition, Vector2f::zero, false).position;
    Vector2 p2 = getDrawParams(position + bezier.endPointOffset.as<f32>(), Vector2f::zero, ctx.cameraPosition, Vector2f::zero, false).position;
    DrawSplineSegmentBezierQuadratic(p1, controlPoint, p2, bezier.thickness * VIRTUAL_SCREEN_RATIO, bezier.color);
}

void BezierRenderSystem::addToQueue(std::vector<EntityRenderInfo>& queue) const {
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto line = entity.get<DrawBezierQuad>();
        const auto trans = entity.get<Transform2D>();
        const Vector2i position = (entity.has<PrecisePosition>() ? entity.get<PrecisePosition>().position : trans.position.as<f32>()).round();

        queue.emplace_back(EntityRenderInfo{
            .boundingBox = AABB::fromPoints(position, position + line.controlPointOffset, position + line.endPointOffset),
            .depth = line.depth,
            .entity = entity,
            .piRender = this,
        });
    }
}

}  // namespace whal
