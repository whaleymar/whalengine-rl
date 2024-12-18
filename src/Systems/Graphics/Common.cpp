#include "Common.h"
#include <cstring>
#include <raylib.h>

#include "Components/Draw.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Settings.h"

namespace whal::gfx {

const DrawMetaData DrawMetaData::NONE = {0, false, false};

rl::Vector3 DrawMetaData::asRL() const {
    u32 packed = 0;
    packed |= static_cast<u32>(depth);

    if (isOccluder) {
        packed |= (1 << 8);
    }

    if (isUI) {
        packed |= (1 << 9);
    }

    f32 x;
    std::memcpy(&x, &packed, sizeof(f32));
    return rl::Vector3{x, 0.0, 0.0};
}

rl::Vector3 DrawMetaData::asRL(const Sprite& sprite, Vector2f textureDims) const {
    u32 packed = 0;
    packed |= static_cast<u32>(depth);

    if (isOccluder) {
        packed |= (1 << 8);
    }

    if (isUI) {
        packed |= (1 << 9);
    }

    Vector2f maskOffsetUV;
    if (!sprite.maskPosRelative.isZero()) {
        // might want to set a flag in the CBI? Idk i guess i can just check if these values are zero
        f32 x = sprite.maskPosRelative.x / textureDims.x;  // x offset
        f32 y = sprite.maskPosRelative.y / textureDims.y;  // y offset

        maskOffsetUV = {x, y};
        packed |= (1 << 10);  // set flag so we know there's a mask
    }

    if (sprite.isFlagSet(Sprite::Silhouette)) {
        packed |= (1 << 11);
    }

    if (sprite.isFlagSet(Sprite::MaskBlendAdditive)) {
        packed |= (1 << 12);
    }

    f32 x;
    std::memcpy(&x, &packed, sizeof(f32));
    return rl::Vector3{x, maskOffsetUV.x, maskOffsetUV.y};
}

bool RenderQueue::add(const EntityPreRenderInfo& renderInfo) {
    if (mCameraViewBox.isOverlapping(renderInfo.boundingBox)) {
        const bool isOccluder = renderInfo.isOccluder == EntityPreRenderInfo::IsOccluder::Unchecked ?
                                    renderInfo.entity.has<BlocksLight>() :
                                    (renderInfo.isOccluder == EntityPreRenderInfo::IsOccluder::Yes ? true : false);
        if (renderInfo.transform.depth == Depth::Debug || renderInfo.transform.depth == Depth::UIFar ||
            renderInfo.transform.depth == Depth::UIClose) {
            mUIQueue.emplace_back(renderInfo.boundingBox.bottom(), renderInfo.transform, mpIRender, renderInfo.entity,
                                  gfx::DrawMetaData{
                                      .depth = static_cast<u8>(renderInfo.transform.depth),
                                      .isOccluder = isOccluder,
                                      .isUI = true,
                                  },
                                  renderInfo.internal);
        } else {
            mNormalQueue.emplace_back(renderInfo.boundingBox.bottom(), renderInfo.transform, mpIRender, renderInfo.entity,
                                      gfx::DrawMetaData{
                                          .depth = static_cast<u8>(renderInfo.transform.depth),
                                          .isOccluder = isOccluder,
                                          .isUI = false,
                                      },
                                      renderInfo.internal);
        }
        return true;
    }
    return false;
}

void RenderQueue::addPrecalculated(const EntityPreRenderInfo& renderInfo) {
    const bool isOccluder = renderInfo.isOccluder == EntityPreRenderInfo::IsOccluder::Unchecked ?
                                renderInfo.entity.has<BlocksLight>() :
                                (renderInfo.isOccluder == EntityPreRenderInfo::IsOccluder::Yes ? true : false);
    if (renderInfo.transform.depth == Depth::Debug || renderInfo.transform.depth == Depth::UIFar || renderInfo.transform.depth == Depth::UIClose) {
        mUIQueue.emplace_back(renderInfo.boundingBox.bottom(), renderInfo.transform, mpIRender, renderInfo.entity,
                              gfx::DrawMetaData{
                                  .depth = static_cast<u8>(renderInfo.transform.depth),
                                  .isOccluder = isOccluder,
                                  .isUI = true,
                              },
                              renderInfo.internal);
    } else {
        mNormalQueue.emplace_back(renderInfo.boundingBox.bottom(), renderInfo.transform, mpIRender, renderInfo.entity,
                                  gfx::DrawMetaData{
                                      .depth = static_cast<u8>(renderInfo.transform.depth),
                                      .isOccluder = isOccluder,
                                      .isUI = false,
                                  },
                                  renderInfo.internal);
    }
}

void clampToPixelGrid(RaylibDrawParams& params) {
    Vector2f positionF = Vector2f(params.position);
    Vector2i position = (positionF / VIRTUAL_SCREEN_RATIO).round() * static_cast<s32>(VIRTUAL_SCREEN_RATIO);

    params.position = position.asRL();
    params.rect.x = params.position.x;
    params.rect.y = params.position.y;
}

RaylibDrawParams getDrawParams(const Transform& transform, Vector2f frameSize, Vector2f cameraPosition) {
    const Vector2f size = frameSize * transform.scale * VIRTUAL_SCREEN_RATIO;
    const Vector2f position = transform.getRotatedPosition();
    const Vector2f screenPosition = Vector2f(position.x - cameraPosition.x, cameraPosition.y - position.y) * VIRTUAL_SCREEN_RATIO +
                                    Vector2f(FWINDOW_WIDTH_RENDER / 2, FWINDOW_HEIGHT_RENDER / 2);
    const Vector2f origin = size * Vector2f(0.5, 0.5);

    return RaylibDrawParams{
        .rect = rl::Rectangle{screenPosition.x, screenPosition.y, size.x, size.y},
        .origin = rl::Vector2{origin.x, origin.y},
        .position = rl::Vector2{screenPosition.x, screenPosition.y},
    };
}

}  // namespace whal::gfx
