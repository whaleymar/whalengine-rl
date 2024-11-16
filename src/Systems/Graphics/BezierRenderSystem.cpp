#include "BezierRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Settings.h"

namespace whal {

void BezierRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    const auto bezier = eCtx.entity.get<DrawBezierQuad>();
    Vector2 p1 = eCtx.preciseTransform.getRotatedPosition().asRL();
    Vector2 controlPoint = eCtx.preciseTransform.apply(bezier.controlPointOffset.as<f32>()).asRL();
    Vector2 p2 = eCtx.preciseTransform.apply(bezier.endPointOffset.as<f32>()).asRL();
    DrawSplineSegmentBezierQuadratic(p1, controlPoint, p2, bezier.thickness * VIRTUAL_SCREEN_RATIO, bezier.color);
}

void BezierRenderSystem::addToQueue(std::vector<gfx::EntityRenderInfo>& queue) const {
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto line = entity.get<DrawBezierQuad>();
        const PreciseTransform2D pTrans = gfx::getPreciseTrans(entity);
        const Vector2i position = pTrans.position.round();

        queue.emplace_back(gfx::EntityRenderInfo{
            .boundingBox = AABB::fromPoints(position, position + line.controlPointOffset, position + line.endPointOffset),
            .preciseTransform = pTrans,
            .entity = entity,
            .piRender = this,
        });
    }
}

}  // namespace whal
