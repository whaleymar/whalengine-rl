#include "LineRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Settings.h"

namespace whal {

struct LinePoints {
    Vector2i p1;
    Vector2i p2;
};

static LinePoints getRotatedPoints(Vector2f position, Transform2D trans, DrawStraightLine line) {
    Vector2i startPos;
    Vector2i endPos;
    if (line.isRotateAboutCenter) {
        Vector2f halfLine = Vector2f::fromAngle(trans.rotationDegrees) * static_cast<f32>(line.length) * 0.5f;
        startPos = (position - halfLine).round();
        endPos = (position + halfLine).round();
    } else {
        startPos = trans.position;
        endPos = trans.position + (Vector2f::fromAngle(trans.rotationDegrees) * static_cast<f32>(line.length)).round();
    }

    return LinePoints{startPos, endPos};
}

void LineRenderSystem::draw(ecs::Entity entity, const gfx::RenderContext ctx) const {
    const auto line = entity.get<DrawStraightLine>();
    PreciseTransform2D pTrans = gfx::getPreciseTrans(entity);

    const LinePoints points = getRotatedPoints(pTrans.position, entity.get<Transform2D>(), line);
    const Vector2 p1 = worldToScreenCoords(points.p1.as<f32>(), ctx.cameraPosition).asRL();
    const Vector2 p2 = worldToScreenCoords(points.p2.as<f32>(), ctx.cameraPosition).asRL();
    DrawLineEx(p1, p2, line.thickness * VIRTUAL_SCREEN_RATIO, line.color);
}

void LineRenderSystem::addToQueue(std::vector<gfx::EntityRenderInfo>& queue) const {
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto line = entity.get<DrawStraightLine>();
        const auto trans = entity.get<Transform2D>();
        const Vector2f position = entity.has<PrecisePosition>() ? entity.get<PrecisePosition>().position : trans.position.as<f32>();
        const LinePoints points = getRotatedPoints(position, trans, line);

        queue.emplace_back(gfx::EntityRenderInfo{
            .boundingBox = AABB::fromPoints(points.p1, points.p2),
            .depth = trans.depth,
            .entity = entity,
            .piRender = this,
        });
    }
}

}  // namespace whal
