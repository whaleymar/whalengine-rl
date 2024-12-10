#include "Common.h"
#include <cstring>
#include <raylib.h>

#include "Components/Collision.h"
#include "Components/Draw.h"
#include "Components/Floating.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Settings.h"

namespace whal::gfx {

const DrawMetaData DrawMetaData::NONE = {Vector2f::ZERO, 0, false, false};

void DrawMetaData::setFlags(const Sprite& sprite, Vector2f textureDims) {
    if (!sprite.maskPosRelative.isZero()) {
        // might want to set a flag in the CBI? Idk i guess i can just check if these values are zero
        f32 x = static_cast<f32>(sprite.maskPosRelative.x) / textureDims.x;  // x offset
        f32 y = static_cast<f32>(sprite.maskPosRelative.y) / textureDims.y;  // y offset

        maskOffsetUV = {x, y};
    }

    if (sprite.isFlagSet(Sprite::Silhouette)) {
        isSilhouette = true;
    }
}

rl::Vector3 DrawMetaData::asRL() const {
    u32 packed = 0;
    packed |= static_cast<u32>(depth);

    if (isOccluder) {
        packed |= (1 << 8);
    }

    if (isUI) {
        packed |= (1 << 9);
    }

    if (maskOffsetUV != Vector2f::ZERO) {
        packed |= (1 << 10);  // set flag so we know there's a mask
    }

    if (isSilhouette) {
        packed |= (1 << 11);
    }

    f32 x;
    std::memcpy(&x, &packed, sizeof(f32));
    return rl::Vector3{x, maskOffsetUV.x, maskOffsetUV.y};
}

void RenderQueue::push_back(const EntityRenderLoc& renderInfo) {
    if (mCameraViewBox.isOverlapping(renderInfo.boundingBox)) {
        if (renderInfo.preciseTransform.depth == Depth::Debug || renderInfo.preciseTransform.depth == Depth::UIFar ||
            renderInfo.preciseTransform.depth == Depth::UIClose) {
            mUIQueue.emplace_back(renderInfo.boundingBox.bottom(), renderInfo.preciseTransform, renderInfo.entity, mpIRender,
                                  gfx::DrawMetaData{.maskOffsetUV = Vector2f::ZERO,
                                                    .depth = static_cast<u8>(renderInfo.preciseTransform.depth),
                                                    .isOccluder = renderInfo.entity.has<BlocksLight>(),
                                                    .isUI = true});
        } else {
            mNormalQueue.emplace_back(renderInfo.boundingBox.bottom(), renderInfo.preciseTransform, renderInfo.entity, mpIRender,
                                      gfx::DrawMetaData{.maskOffsetUV = Vector2f::ZERO,
                                                        .depth = static_cast<u8>(renderInfo.preciseTransform.depth),
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
