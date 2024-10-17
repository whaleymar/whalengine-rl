#include "RectangleRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Physics/Box.h"

namespace whal {

void RectangleRenderSystem::draw(ecs::Entity entity, const RenderContext ctx) const {
    const DrawRect rect = entity.get<DrawRect>();
    const Transform2D trans = entity.get<Transform2D>();

    PreciseTransform2D pTrans = PreciseTransform2D::fromTrans(trans);
    if (entity.has<PreciseTransform2D>()) {
        pTrans.position = entity.get<PrecisePosition>().position;
    }

    const auto frameSize = rect.getFrameSize().as<f32>();
    const RaylibDrawParams params = getDrawParamsNew(pTrans, frameSize, ctx.cameraPosition);
    const Color color = ctx.colorOverride ? *ctx.colorOverride : rect.color;
    DrawRectanglePro(params.rect, params.origin, pTrans.rotationDegrees, color);
}

void RectangleRenderSystem::addToQueue(std::vector<EntityRenderInfo>& queue) const {
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto draw = entity.get<DrawRect>();
        const auto trans = entity.get<Transform2D>();
        const auto bb = trans.rotationDegrees == 0.0f ?
                            AABB(entity.get<Transform2D>(), draw.getFrameSize() / 2) :
                            Box(trans.getRotatedPosition(), draw.getFrameSize() / 2, trans.rotationDegrees).getBoundingAABB();

        queue.emplace_back(EntityRenderInfo{
            .boundingBox = bb,
            .depth = draw.depth,
            .entity = entity,
            .piRender = this,
        });
    }
}

}  // namespace whal
