#include "LineRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Gfx/Coordinates.h"
#include "Gfx/RaylibUtil.h"
#include "Settings.h"
#include "Sys/System.h"
#include "Sys/Time.h"
#include "Util/MathUtil.h"

namespace whal {

struct LinePoints {
    Vector2f p1;
    Vector2f p2;
};

static LinePoints getRotatedPoints(Vector2f position, Transform trans, DrawStraightLine line) {
    Vector2f startPos;
    Vector2f endPos;
    if (line.isRotateAboutCenter) {
        Vector2f halfLine = Vector2f::fromAngle(trans.rotation) * static_cast<f32>(line.length) * 0.5f;
        startPos = position - halfLine;
        endPos = position + halfLine;
    } else {
        startPos = trans.position;
        endPos = trans.position + Vector2f::fromAngle(trans.rotation) * static_cast<f32>(line.length);
    }

    return LinePoints{startPos, endPos};
}

void LineRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    const auto line = eCtx.entity.get<DrawStraightLine>();
    const LinePoints points = getRotatedPoints(eCtx.transform->position, eCtx.entity.get<Transform>(), line);
    const Vector2f p1 = worldToRenderCoords(points.p1);
    const Vector2f p2 = worldToRenderCoords(points.p2);

    const f32 len = (p2 - p1).len();
    if (math::isNearZero(len, 0.01)) {
        return;
    }

    if (line.segmentLength == 0 || line.segmentGapLength == 0) {
        // draw as single segment
        gfx::DrawLineHDR(p1.asRL(), p2.asRL(), line.thickness * VIRTUAL_SCREEN_RATIO, line.color, eCtx.colorBuf);
        return;
    }

    const Vector2f norm = (p2 - p1) / len;
    const Vector2f segmentStep = norm * static_cast<f32>(line.segmentLength) * VIRTUAL_SCREEN_RATIO;
    const Vector2f gapStep = norm * static_cast<f32>(line.segmentGapLength) * VIRTUAL_SCREEN_RATIO;
    Vector2f current = p1;
    if (line.segmentCycleTime > 0.0) {
        f32 offsetT = std::fmod(Time.getElapsed(), line.segmentCycleTime) / line.segmentCycleTime;
        current += (segmentStep + gapStep) * offsetT;
    }

    while (true) {
        // check if we're past p2
        if (math::sign(p2.x - p1.x) != math::sign(p2.x - current.x) || math::sign(p2.y - p1.y) != math::sign(p2.y - current.y)) {
            return;
        }

        Vector2f next = current + segmentStep;
        if (math::sign(p2.x - p1.x) != math::sign(p2.x - next.x) || math::sign(p2.y - p1.y) != math::sign(p2.y - next.y)) {
            // don't draw beyond p2
            gfx::DrawLineHDR(current.asRL(), p2.asRL(), line.thickness * VIRTUAL_SCREEN_RATIO, line.color, eCtx.colorBuf);
            return;
        }

        gfx::DrawLineHDR(current.asRL(), next.asRL(), line.thickness * VIRTUAL_SCREEN_RATIO, line.color, eCtx.colorBuf);
        current += segmentStep + gapStep;
    }
}

void LineRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    for (const auto& [entityid, entity] : getEntities()) {
        const DrawStraightLine& line = entity.get<DrawStraightLine>();
        const Transform& trans = entity.get<Transform>();
        const LinePoints points = getRotatedPoints(trans.position, trans, line);
        AABB bb = AABB::fromPoints(points.p1.as<s32>(), points.p2.as<s32>());

        // float height should affect bounding box (for culling) but not Y sorting
        s32 floatOffset = static_cast<s32>(trans.floatHeight * FLOAT_HEIGHT_MULT);
        bb.getPositionMut().y += floatOffset;
        queue.add(gfx::EntityPreRenderInfo{
            .boundingBox = bb,
            .transform = &trans,
            .ysortPosition = bb.bottom() - floatOffset,
            .entity = entity,
        });
    }
}

}  // namespace whal
