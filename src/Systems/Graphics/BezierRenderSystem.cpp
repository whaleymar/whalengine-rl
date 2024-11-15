#include "BezierRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Settings.h"

namespace whal {

void BezierRenderSystem::draw(ecs::Entity entity, const gfx::RenderContext ctx) const {
    const auto bezier = entity.get<DrawBezierQuad>();
    const Transform2D trans = entity.get<Transform2D>();
    const PreciseTransform2D pTrans = gfx::getPreciseTrans(entity);

    Vector2 p1 = pTrans.getRotatedPosition().asRL();
    Vector2 controlPoint = pTrans.apply(bezier.controlPointOffset.as<f32>()).asRL();
    Vector2 p2 = pTrans.apply(bezier.endPointOffset.as<f32>()).asRL();
    DrawSplineSegmentBezierQuadratic(p1, controlPoint, p2, bezier.thickness * VIRTUAL_SCREEN_RATIO, bezier.color);
}

void BezierRenderSystem::addToQueue(std::vector<gfx::EntityRenderInfo>& queue) const {
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto line = entity.get<DrawBezierQuad>();
        const auto trans = entity.get<Transform2D>();
        const Vector2i position = (entity.has<PrecisePosition>() ? entity.get<PrecisePosition>().position : trans.position.as<f32>()).round();

        queue.emplace_back(gfx::EntityRenderInfo{
            .boundingBox = AABB::fromPoints(position, position + line.controlPointOffset, position + line.endPointOffset),
            .depth = trans.depth,
            .entity = entity,
            .piRender = this,
        });
    }
}

}  // namespace whal
