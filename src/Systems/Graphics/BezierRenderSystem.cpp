#include "BezierRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Gfx/RaylibUtil.h"
#include "Settings.h"

namespace whal {

void BezierRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    const auto bezier = eCtx.entity.get<DrawBezierQuad>();
    rl::Vector2 p1 = eCtx.preciseTransform.getRotatedPosition().asRL();
    rl::Vector2 controlPoint = eCtx.preciseTransform.apply(bezier.controlPointOffset.as<f32>()).asRL();
    rl::Vector2 p2 = eCtx.preciseTransform.apply(bezier.endPointOffset.as<f32>()).asRL();

    gfx::DrawSplineSegmentBezierQuadraticHDR(p1, controlPoint, p2, bezier.thickness * VIRTUAL_SCREEN_RATIO, bezier.color, eCtx.colorBuf);
}

void BezierRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    for (const auto& [entityid, entity] : getEntities()) {
        const auto line = entity.get<DrawBezierQuad>();
        const PreciseTransform pTrans = gfx::getPreciseTrans(entity);
        const Vector2i position = pTrans.position.round();

        queue.add(gfx::EntityPreRenderInfo{
            .boundingBox = AABB::fromPoints(position, position + line.controlPointOffset, position + line.endPointOffset),
            .preciseTransform = pTrans,
            .entity = entity,
        });
    }
}

}  // namespace whal
