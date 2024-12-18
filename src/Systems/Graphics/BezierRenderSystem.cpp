#include "BezierRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Gfx/RaylibUtil.h"
#include "Settings.h"

namespace whal {

void BezierRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    const auto bezier = eCtx.entity.get<DrawBezierQuad>();
    rl::Vector2 p1 = eCtx.transform.getRotatedPosition().asRL();
    rl::Vector2 controlPoint = eCtx.transform.apply(bezier.controlPointOffset.as<f32>()).asRL();
    rl::Vector2 p2 = eCtx.transform.apply(bezier.endPointOffset.as<f32>()).asRL();

    gfx::DrawSplineSegmentBezierQuadraticHDR(p1, controlPoint, p2, bezier.thickness * VIRTUAL_SCREEN_RATIO, bezier.color, eCtx.colorBuf);
}

void BezierRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    for (const auto& [entityid, entity] : getEntities()) {
        const auto line = entity.get<DrawBezierQuad>();
        const Transform& trans = entity.get<Transform>();

        queue.add(gfx::EntityPreRenderInfo{
            .boundingBox = AABB::fromPoints(trans.positionPx, trans.positionPx + line.controlPointOffset, trans.positionPx + line.endPointOffset),
            .transform = trans,
            .entity = entity,
        });
    }
}

}  // namespace whal
