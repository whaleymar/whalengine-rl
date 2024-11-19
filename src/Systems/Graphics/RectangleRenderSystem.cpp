#include "RectangleRenderSystem.h"
#include <raylib.h>

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/ShaderManager.h"
#include "Physics/Box.h"

namespace whal {

void RectangleRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    const DrawRect rect = eCtx.entity.get<DrawRect>();

    const auto frameSize = rect.frameSize.as<f32>();
    const gfx::RaylibDrawParams params = gfx::getDrawParams(eCtx.preciseTransform, frameSize, ctx.cameraPosition);

    // If we don't deactivate, we minimize the number of shader swaps.
    // Swaps only happen if the new shader isn't the active one.
    BeginShaderMode(ShaderManager::get(Shaders::Default));
    gfx::DrawRectangleHDR(params.rect, params.origin, eCtx.preciseTransform.rotationDegrees, rect.color, rect.brightness, eCtx.colorBuf);
}

void RectangleRenderSystem::addToQueue(std::vector<gfx::EntityRenderInfo>& queue) const {
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto draw = entity.get<DrawRect>();
        const auto trans = entity.get<Transform2D>();
        const auto pTrans = gfx::getPreciseTrans(entity, trans);
        const auto bb = trans.rotationDegrees == 0.0f ? AABB(trans, draw.frameSize / 2, Vector2i()) :
                                                        Box(trans.getRotatedPosition(), draw.frameSize / 2, trans.rotationDegrees).getBoundingAABB();

        queue.emplace_back(gfx::EntityRenderInfo{
            .boundingBox = bb,
            .preciseTransform = pTrans,
            .entity = entity,
            .piRender = this,
        });
    }
}

}  // namespace whal
