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

    // if (isOccluder) {
    //     packed |= (1 << 8);
    // }

    if (isUI) {
        packed |= (1 << 9);
    }

    f32 x;
    std::memcpy(&x, &packed, sizeof(f32));
    return rl::Vector3{x, 0.0, 0.0};
}

rl::Vector3 DrawMetaData::asRL(const Sprite& sprite, Vector2f textureDims) const {
    u32 packed = static_cast<u32>(depth);

    // if (isOccluder) {
    //     packed |= (1 << 8);
    // }

    if (isUI) {
        packed |= (1 << 9);
    }

    f32 maskOffsetUVX = 0.0f;
    f32 maskOffsetUVY = 0.0f;
    if (!sprite.maskPosRelative.isZero()) {
        maskOffsetUVX = sprite.maskPosRelative.x / textureDims.x;  // x offset
        maskOffsetUVY = sprite.maskPosRelative.y / textureDims.y;  // y offset

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
    return rl::Vector3{x, maskOffsetUVX, maskOffsetUVY};
}

bool RenderQueue::add(const EntityPreRenderInfo& renderInfo) {
    bool isDraw = false;
    if (mCameraViewBox.isOverlapping(renderInfo.boundingBox)) {
        bool isOccluder = addPrecalculated(renderInfo);
        isDraw = true;

        bool isOccluderMaybe = isOccluder || renderInfo.isOccluder == EntityPreRenderInfo::IsOccluder::MaybeInChildren;
        if (isOccluderMaybe) {
            mOccluderQueueCamera.emplace_back(renderInfo.transform, renderInfo.ysortPosition, renderInfo.entity, mpIRender,
                                              gfx::DrawMetaData{
                                                  .depth = static_cast<u8>(renderInfo.transform->depth),
                                                  .isOccluder = true,
                                                  .isUI = true,
                                              },
                                              renderInfo.internal, renderInfo.shader);
        }

        if (isOccluderMaybe && mGlobalIlluminationViewBox.isOverlapping(renderInfo.boundingBox)) {
            mOccluderQueue.emplace_back(renderInfo.transform, renderInfo.ysortPosition, renderInfo.entity, mpIRender,
                                        gfx::DrawMetaData{
                                            .depth = static_cast<u8>(renderInfo.transform->depth),
                                            .isOccluder = true,
                                            .isUI = true,
                                        },
                                        renderInfo.internal, renderInfo.shader);
        }
    } else if (mGlobalIlluminationViewBox.isOverlapping(renderInfo.boundingBox)) {
        bool isOccluder;
        switch (renderInfo.isOccluder) {
        case EntityPreRenderInfo::IsOccluder::Unchecked:
            isOccluder = renderInfo.entity.has<BlocksLight>();
            break;
        case EntityPreRenderInfo::IsOccluder::Yes:
        case EntityPreRenderInfo::IsOccluder::MaybeInChildren:
            isOccluder = true;
            break;
        case EntityPreRenderInfo::IsOccluder::No:
            isOccluder = false;
            break;
        }
        if (isOccluder) {
            mOccluderQueue.emplace_back(renderInfo.transform, renderInfo.ysortPosition, renderInfo.entity, mpIRender,
                                        gfx::DrawMetaData{
                                            .depth = static_cast<u8>(renderInfo.transform->depth),
                                            .isOccluder = true,
                                            .isUI = true,
                                        },
                                        renderInfo.internal, renderInfo.shader);
        }
    }
    return isDraw;
}

bool RenderQueue::addPrecalculated(const EntityPreRenderInfo& renderInfo) {
    bool isOccluder;
    switch (renderInfo.isOccluder) {
    case EntityPreRenderInfo::IsOccluder::Unchecked:
        isOccluder = renderInfo.entity.has<BlocksLight>();
        break;
    case EntityPreRenderInfo::IsOccluder::Yes:
        isOccluder = true;
        break;
    case EntityPreRenderInfo::IsOccluder::MaybeInChildren:
    case EntityPreRenderInfo::IsOccluder::No:
        isOccluder = false;
        break;
    }
    if (renderInfo.transform->depth == Depth::Debug || renderInfo.transform->depth == Depth::UIFar || renderInfo.transform->depth == Depth::UIClose) {
        mUIQueue.emplace_back(renderInfo.transform, renderInfo.ysortPosition, renderInfo.entity, mpIRender,
                              gfx::DrawMetaData{
                                  .depth = static_cast<u8>(renderInfo.transform->depth),
                                  .isOccluder = isOccluder,
                                  .isUI = true,
                              },
                              renderInfo.internal, renderInfo.shader);
    } else {
        mNormalQueue.emplace_back(renderInfo.transform, renderInfo.ysortPosition, renderInfo.entity, mpIRender,
                                  gfx::DrawMetaData{
                                      .depth = static_cast<u8>(renderInfo.transform->depth),
                                      .isOccluder = isOccluder,
                                      .isUI = false,
                                  },
                                  renderInfo.internal, renderInfo.shader);
    }
    return isOccluder;
}

void RenderQueue::clear() {
    mNormalQueue.clear();
    mUIQueue.clear();
    mOccluderQueue.clear();
    mOccluderQueueCamera.clear();
}

void clampToPixelGrid(RaylibDrawParams& params) {
    Vector2f positionF = Vector2f(params.rect.x, params.rect.y);
    Vector2i position = (positionF / VIRTUAL_SCREEN_RATIO).round() * static_cast<s32>(VIRTUAL_SCREEN_RATIO);

    params.rect.x = position.x;
    params.rect.y = position.y;
}

RaylibDrawParams getDrawParams(const Transform& transform, Vector2f frameSize) {
    const Vector2f size = (frameSize * transform.scale * VIRTUAL_SCREEN_RATIO).absolute();
    const Vector2f screenPosition = transform.getRotatedPosition() * Vector2f(VIRTUAL_SCREEN_RATIO, -VIRTUAL_SCREEN_RATIO);
    const Vector2f origin = size * Vector2f(0.5, 0.5);

    return RaylibDrawParams{
        .rect = rl::Rectangle{screenPosition.x, screenPosition.y, size.x, size.y},
        .origin = rl::Vector2{origin.x, origin.y},
    };
}

Vector2f getGISector(s32 sector) {
    // clang-format off
    auto const size = getGIPadding();
    switch (sector) {
        case 0: return {0,0};
        case 1: return {size.x, 0};
        case 2: return {size.x + FWINDOW_WIDTH_GAME , 0};
        case 3: return {size.x + FWINDOW_WIDTH_GAME , size.y};
        case 4: return {size.x + FWINDOW_WIDTH_GAME , size.y + FWINDOW_HEIGHT_GAME };
        case 5: return {size.x, size.y + FWINDOW_HEIGHT_GAME};
        case 6: return {0, size.y + FWINDOW_HEIGHT_GAME};
        case 7: return {0, size.y};
        case 8: return {size.x, size.y};
        default: return {0,0};
    }
    // clang-format on
}

Vector2f getGISectorOffset(s32 sector) {
    return (getGISector(sector) - getGISector(8)) * Vector2f(1, -1);
}

Vector2f getGISectorSize(s32 sector) {
    // clang-format off
    Vector2f viewSize = {FWINDOW_WIDTH_GAME, FWINDOW_HEIGHT_GAME};
    Vector2f padSize = getGIPadding();
    switch (sector) {
        case 0: return padSize;
        case 1: return {viewSize.x, padSize.y};
        case 2: return  padSize;
        case 3: return {padSize.x, viewSize.y};
        case 4: return  padSize;
        case 5: return {viewSize.x, padSize.y};
        case 6: return padSize;
        case 7: return {padSize.x, viewSize.y};
        case 8: return viewSize;
        default: return padSize;
    }
    // clang-format on
}

Vector2f getGIPadding() {
    return {FWINDOW_WIDTH_GAME / 2.0f, FWINDOW_HEIGHT_GAME / 2.0f};
}

AABB getGIViewBox(Vector2i cameraPosition, s32 sector) {
    Vector2i giSectorTopLeft =
        Vector2i(cameraPosition.x - WINDOW_WIDTH_GAME / 2, cameraPosition.y + WINDOW_HEIGHT_GAME / 2) + gfx::getGISectorOffset(sector).as<s32>();
    Vector2i giSectorSizeHalf = gfx::getGISectorSize(sector).as<s32>() / 2;
    Vector2i giSectorCenter = giSectorTopLeft + giSectorSizeHalf * Vector2i(1, -1);
    Vector2i giSectorSize = giSectorSizeHalf + Vector2i(PIXELS_PER_TILE, PIXELS_PER_TILE);
    if (EDITOR_FRAME_DO_NEXT) {
        print("GI sector", sector, "\n\tsize:", gfx::getGISectorSize(sector), "\n\ttop left:", giSectorTopLeft, "\n\tcenter:", giSectorCenter,
              "\n\thalf:", giSectorSize);
        print("");
    }
    return AABB(giSectorCenter, giSectorSize);
}

}  // namespace whal::gfx
