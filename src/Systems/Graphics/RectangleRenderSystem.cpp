#include "RectangleRenderSystem.h"
#include <raylib.h>

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Gfx/RaylibUtil.h"
#include "Physics/Box.h"
#include "Settings.h"

namespace whal {

void RectangleRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    const DrawRect rect = eCtx.entity.get<DrawRect>();

    const auto frameSize = rect.frameSize.as<f32>();
    const gfx::RaylibDrawParams params = gfx::getDrawParams(*eCtx.transform, frameSize);

    gfx::DrawRectangleHDR(params.rect, params.origin, eCtx.transform->rotation, rect.color, eCtx.colorBuf);
}

void RectangleRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    for (const auto& [entityid, entity] : getEntities()) {
        const DrawRect& draw = entity.get<DrawRect>();
        const Transform& trans = entity.get<Transform>();
        const AABB bb = trans.rotation == 0.0f ? AABB(trans, draw.frameSize / 2, Vector2i()) :
                                                 Box(trans.getRotatedPosition().round(), draw.frameSize / 2, trans.rotation).getBoundingAABB();

        // for Y sorting purposes I want floatHeight to be ignored
        queue.add(gfx::EntityPreRenderInfo{
            .boundingBox = bb,
            .transform = &trans,
            .ysortPosition = bb.bottom() - static_cast<s32>(trans.floatHeight * FLOAT_HEIGHT_MULT),
            .entity = entity,
        });
    }
}

}  // namespace whal
