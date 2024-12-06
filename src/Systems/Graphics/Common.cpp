#include "Common.h"
#include <cstring>
#include <raylib.h>

#include "Components/Collision.h"
#include "Components/Floating.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Settings.h"

namespace whal::gfx {

const ColorBufInfo ColorBufInfo::NONE = {0, false, false};

rl::Vector3 ColorBufInfo::asRL() const {
    u32 packed = 0;
    packed |= static_cast<u32>(depth);

    if (isOccluder) {
        packed |= (1 << 8);
    }

    if (isUI) {
        packed |= (1 << 9);
    }

    f32 result;
    std::memcpy(&result, &packed, sizeof(f32));
    return rl::Vector3{result, 0.0f, 0.0f};
}

void RenderQueue::push_back(const EntityRenderLoc& renderInfo) {
    if (mCameraViewBox.isOverlapping(renderInfo.boundingBox)) {
        if (renderInfo.preciseTransform.depth == Depth::Debug || renderInfo.preciseTransform.depth == Depth::UIFar ||
            renderInfo.preciseTransform.depth == Depth::UIClose) {
            mUIQueue.emplace_back(renderInfo.boundingBox.bottom(), renderInfo.preciseTransform, renderInfo.entity, mpIRender,
                                  gfx::ColorBufInfo{.depth = static_cast<u8>(renderInfo.preciseTransform.depth),
                                                    .isOccluder = renderInfo.entity.has<BlocksLight>(),
                                                    .isUI = true});
        } else {
            mNormalQueue.emplace_back(renderInfo.boundingBox.bottom(), renderInfo.preciseTransform, renderInfo.entity, mpIRender,
                                      gfx::ColorBufInfo{.depth = static_cast<u8>(renderInfo.preciseTransform.depth),
                                                        .isOccluder = renderInfo.entity.has<BlocksLight>(),
                                                        .isUI = false});
        }
    }
}

PreciseTransform getPreciseTrans(ecs::Entity entity) {
    assert(entity.has<Transform>());

    PreciseTransform pTrans = PreciseTransform::fromTrans(entity.get<Transform>());
    if (entity.has<PreciseTransform>()) {
        pTrans.position = entity.get<PrecisePosition>().position;
    } else if (entity.has<Collider>()) {
        // Make physics movement look smooth even though it's pixel perfect
        pTrans.position += entity.get<Collider>().getRemainder();
    }
    if (entity.has<Floating>()) {
        pTrans.floatHeight = entity.get<Floating>().height;
    }
    return pTrans;
}

PreciseTransform getPreciseTrans(ecs::Entity entity, const Transform& transform) {
    PreciseTransform pTrans = PreciseTransform::fromTrans(transform);
    if (entity.has<PreciseTransform>()) {
        pTrans.position = entity.get<PrecisePosition>().position;
    } else if (entity.has<Collider>()) {
        // Make physics movement look smooth even though it's pixel perfect
        pTrans.position += entity.get<Collider>().getRemainder();
    }
    if (entity.has<Floating>()) {
        pTrans.floatHeight = entity.get<Floating>().height;
    }
    return pTrans;
}

void clampToPixelGrid(RaylibDrawParams& params) {
    Vector2f positionF = Vector2f(params.position);
    Vector2i position = (positionF / VIRTUAL_SCREEN_RATIO).round() * static_cast<s32>(VIRTUAL_SCREEN_RATIO);

    params.position = position.asRL();
    params.rect.x = params.position.x;
    params.rect.y = params.position.y;
}

RaylibDrawParams getDrawParams(PreciseTransform transform, Vector2f frameSize, Vector2f cameraPosition) {
    const Vector2f size = frameSize * transform.scale * VIRTUAL_SCREEN_RATIO;
    const Vector2f positionF = transform.getRotatedPosition();
    const Vector2f screenPosition = Vector2f(positionF.x - cameraPosition.x, cameraPosition.y - positionF.y) * VIRTUAL_SCREEN_RATIO +
                                    Vector2f(FWINDOW_WIDTH_RENDER / 2, FWINDOW_HEIGHT_RENDER / 2);
    const Vector2f origin = size * Vector2f(0.5, 0.5);

    return RaylibDrawParams{
        .rect = rl::Rectangle{screenPosition.x, screenPosition.y, size.x, size.y},
        .origin = rl::Vector2{origin.x, origin.y},
        .position = rl::Vector2{screenPosition.x, screenPosition.y},
    };
}

}  // namespace whal::gfx
