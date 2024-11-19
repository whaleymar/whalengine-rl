#include "LineRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/ShaderManager.h"
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

void LineRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    const auto line = eCtx.entity.get<DrawStraightLine>();
    const LinePoints points = getRotatedPoints(eCtx.preciseTransform.position, eCtx.entity.get<Transform2D>(), line);
    const Vector2 p1 = worldToScreenCoords(points.p1.as<f32>(), ctx.cameraPosition).asRL();
    const Vector2 p2 = worldToScreenCoords(points.p2.as<f32>(), ctx.cameraPosition).asRL();

    // If we don't deactivate, we minimize the number of shader swaps.
    // Swaps only happen if the new shader isn't the active one.
    BeginShaderMode(ShaderManager::get(Shaders::Default));
    gfx::DrawLineHDR(p1, p2, line.thickness * VIRTUAL_SCREEN_RATIO, line.color, line.brightness, eCtx.colorBuf);
}

void LineRenderSystem::addToQueue(std::vector<gfx::EntityRenderInfo>& queue) const {
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto line = entity.get<DrawStraightLine>();
        const auto trans = entity.get<Transform2D>();
        const auto pTrans = gfx::getPreciseTrans(entity, trans);
        const LinePoints points = getRotatedPoints(pTrans.position, trans, line);

        queue.emplace_back(gfx::EntityRenderInfo{
            .boundingBox = AABB::fromPoints(points.p1, points.p2),
            .preciseTransform = pTrans,
            .entity = entity,
            .piRender = this,
        });
    }
}

}  // namespace whal
