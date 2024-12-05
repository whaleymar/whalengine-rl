#include "LineRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Gfx/Coordinates.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/ShaderManager.h"
#include "Settings.h"

namespace whal {

struct LinePoints {
    Vector2f p1;
    Vector2f p2;
};

static LinePoints getRotatedPoints(Vector2f position, Transform trans, DrawStraightLine line) {
    Vector2f startPos;
    Vector2f endPos;
    if (line.isRotateAboutCenter) {
        Vector2f halfLine = Vector2f::fromAngle(trans.rotationDegrees) * static_cast<f32>(line.length) * 0.5f;
        startPos = position - halfLine;
        endPos = position + halfLine;
    } else {
        startPos = trans.position.as<f32>();
        endPos = trans.position.as<f32>() + Vector2f::fromAngle(trans.rotationDegrees) * static_cast<f32>(line.length);
    }

    return LinePoints{startPos, endPos};
}

void LineRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    const auto line = eCtx.entity.get<DrawStraightLine>();
    const LinePoints points = getRotatedPoints(eCtx.preciseTransform.position, eCtx.entity.get<Transform>(), line);
    const rl::Vector2 p1 = worldToScreenCoords(points.p1, ctx.cameraPosition).asRL();
    const rl::Vector2 p2 = worldToScreenCoords(points.p2, ctx.cameraPosition).asRL();

    // If we don't deactivate, we minimize the number of shader swaps.
    // Swaps only happen if the new shader isn't the active one.
    BeginShaderMode(ShaderManager::get(Shaders::Default));
    gfx::DrawLineHDR(p1, p2, line.thickness * VIRTUAL_SCREEN_RATIO, line.color, eCtx.colorBuf);
}

void LineRenderSystem::addToQueue(std::vector<gfx::EntityRenderInfo>& queue) const {
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto line = entity.get<DrawStraightLine>();
        const auto trans = entity.get<Transform>();
        const auto pTrans = gfx::getPreciseTrans(entity, trans);
        const LinePoints points = getRotatedPoints(pTrans.position, trans, line);

        queue.emplace_back(gfx::EntityRenderInfo{
            .boundingBox = AABB::fromPoints(points.p1.as<s32>(), points.p2.as<s32>()),
            .preciseTransform = pTrans,
            .entity = entity,
            .piRender = this,
        });
    }
}

}  // namespace whal
