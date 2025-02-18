#include "RectangleRenderSystem.h"
#include <raylib.h>

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Gfx/RaylibUtil.h"
#include "Physics/Box.h"

namespace whal {

void RectangleRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    const DrawRect rect = eCtx.entity.get<DrawRect>();

    const auto frameSize = rect.frameSize.as<f32>();
    const gfx::RaylibDrawParams params = gfx::getDrawParams(*eCtx.transform, frameSize);

    gfx::DrawRectangleHDR(params.rect, params.origin, eCtx.transform->rotation, rect.color, eCtx.colorBuf);
}

void RectangleRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    for (const auto& [entityid, entity] : getEntities()) {
        const auto draw = entity.get<DrawRect>();
        const auto& trans = entity.get<Transform>();
        const auto bb = trans.rotation == 0.0f ? AABB(trans, draw.frameSize / 2, Vector2i()) :
                                                 Box(trans.getRotatedPosition().round(), draw.frameSize / 2, trans.rotation).getBoundingAABB();

        queue.add(gfx::EntityPreRenderInfo{
            .boundingBox = bb,
            .transform = &trans,
            .entity = entity,
        });
    }
}

}  // namespace whal
